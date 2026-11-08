#include "ir.h"

#if CONFIG_IDF_TARGET_ESP32S3

#    include <freertos/task.h>
#    include <new>

namespace lilka {
namespace mod {

static const uint32_t RMT_SRC_HZ = 80000000; // APB
static const uint8_t RMT_DIV = 80; // 1 tick = 1 us
static const uint16_t MAX_TICKS = 32767;

Ir::~Ir() {
    end();
}

esp_err_t Ir::begin(const Config& config) {
    end();
    cfg = config;
    esp_err_t err;
    if (cfg.tx >= 0) {
        rmt_config_t tx = RMT_DEFAULT_CONFIG_TX((gpio_num_t)cfg.tx, cfg.txChannel);
        tx.clk_div = RMT_DIV;
        tx.tx_config.carrier_en = true;
        tx.tx_config.carrier_freq_hz = cfg.carrierHz;
        tx.tx_config.carrier_duty_percent = cfg.dutyPercent;
        tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
        tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        tx.tx_config.idle_output_en = true;
        err = rmt_config(&tx);
        if (err == ESP_OK) err = rmt_driver_install(cfg.txChannel, 0, 0);
        if (err != ESP_OK) return err;
        txItems = new (std::nothrow) rmt_item32_t[MAX_DURATIONS / 2 + 1];
        if (!txItems) {
            end();
            return ESP_ERR_NO_MEM;
        }
        txReady = true;
        currentCarrier = cfg.carrierHz;
    }
    if (cfg.rx >= 0) {
        rmt_config_t rx = RMT_DEFAULT_CONFIG_RX((gpio_num_t)cfg.rx, cfg.rxChannel);
        rx.clk_div = RMT_DIV;
        rx.mem_block_num = 4;
        rx.rx_config.filter_en = true;
        rx.rx_config.filter_ticks_thresh = 200; // in APB ticks: drop glitches < 2.5 us
        rx.rx_config.idle_threshold = cfg.rxIdleUs;
        err = rmt_config(&rx);
        if (err == ESP_OK) err = rmt_driver_install(cfg.rxChannel, 4096, 0);
        if (err == ESP_OK) err = rmt_get_ringbuf_handle(cfg.rxChannel, &rxBuffer);
        if (err == ESP_OK) {
            rxReady = true;
            err = rmt_rx_start(cfg.rxChannel, true);
        }
        if (err != ESP_OK) {
            end();
            return err;
        }
    }
    return ESP_OK;
}

void Ir::end() {
    if (txReady) {
        rmt_driver_uninstall(cfg.txChannel);
        txReady = false;
    }
    if (rxReady) {
        rmt_rx_stop(cfg.rxChannel);
        rmt_driver_uninstall(cfg.rxChannel);
        rxReady = false;
        rxBuffer = nullptr;
    }
    delete[] txItems;
    txItems = nullptr;
}

esp_err_t Ir::sendRaw(const uint16_t* durations, size_t count, uint32_t carrierHz) {
    if (!txReady) return ESP_ERR_INVALID_STATE;
    if (count == 0) return ESP_OK;
    if (count > MAX_DURATIONS) return ESP_ERR_INVALID_SIZE;
    if (carrierHz == 0) carrierHz = cfg.carrierHz;
    if (carrierHz != currentCarrier) {
        uint32_t period = RMT_SRC_HZ / carrierHz;
        uint16_t high = period * cfg.dutyPercent / 100;
        esp_err_t err = rmt_set_tx_carrier(cfg.txChannel, true, high, period - high, RMT_CARRIER_LEVEL_HIGH);
        if (err != ESP_OK) return err;
        currentCarrier = carrierHz;
    }
    size_t items = 0;
    for (size_t i = 0; i < count; i += 2) {
        rmt_item32_t& it = txItems[items++];
        it.level0 = 1;
        it.duration0 = durations[i] > MAX_TICKS ? MAX_TICKS : durations[i];
        it.level1 = 0;
        uint16_t space = i + 1 < count ? durations[i + 1] : 0;
        it.duration1 = space > MAX_TICKS ? MAX_TICKS : space;
    }
    if (count % 2 == 0) {
        txItems[items++].val = 0; // end marker
    }
    return rmt_write_items(cfg.txChannel, txItems, items, true);
}

esp_err_t Ir::send(const IrCode& code, uint8_t repeats) {
    uint16_t buf[80];
    size_t n = IrCodec::encode(code, buf, 80);
    if (!n) return ESP_ERR_NOT_SUPPORTED;
    uint32_t gap = IrCodec::repeatGapMs(code.protocol);
    for (int i = 0; i <= repeats; i++) {
        if (i) vTaskDelay(pdMS_TO_TICKS(gap));
        size_t len = n;
        if (i && code.protocol == IrProtocol::NEC) len = IrCodec::encodeNecRepeat(buf, 80);
        esp_err_t err = sendRaw(buf, len);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

size_t Ir::receiveRaw(uint16_t* out, size_t max) {
    if (!rxReady) return 0;
    for (;;) {
        size_t size = 0;
        rmt_item32_t* items = static_cast<rmt_item32_t*>(xRingbufferReceive(rxBuffer, &size, 0));
        if (!items) return 0;
        size_t count = size / sizeof(rmt_item32_t);
        size_t n = 0;
        bool started = false;
        int lastMark = -1;
        for (size_t i = 0; i < count && n < max; i++) {
            for (int half = 0; half < 2 && n < max; half++) {
                uint16_t dur = half ? items[i].duration1 : items[i].duration0;
                int level = half ? items[i].level1 : items[i].level0;
                if (dur == 0) {
                    i = count; // end of frame
                    break;
                }
                bool mark = cfg.rxActiveLow ? level == 0 : level == 1;
                if (!started) {
                    if (!mark) continue;
                    started = true;
                }
                if ((int)mark == lastMark && n) {
                    uint32_t sum = (uint32_t)out[n - 1] + dur;
                    out[n - 1] = sum > 0xFFFF ? 0xFFFF : sum;
                } else {
                    out[n++] = dur;
                    lastMark = mark;
                }
            }
        }
        vRingbufferReturnItem(rxBuffer, items);
        // Trailing space (if any) is the idle timeout, not part of the frame.
        if (n && n % 2 == 0) n--;
        if (n >= 3) return n; // anything shorter is noise
    }
}

} // namespace mod
} // namespace lilka

#endif // CONFIG_IDF_TARGET_ESP32S3
