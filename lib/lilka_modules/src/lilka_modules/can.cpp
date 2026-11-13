#include "can.h"

namespace lilka {
namespace mod {

Can::~Can() {
    end();
}

bool Can::timingFor(uint32_t bitrate, twai_timing_config_t& out) {
    switch (bitrate) {
        case 25000:
            out = TWAI_TIMING_CONFIG_25KBITS();
            return true;
        case 50000:
            out = TWAI_TIMING_CONFIG_50KBITS();
            return true;
        case 100000:
            out = TWAI_TIMING_CONFIG_100KBITS();
            return true;
        case 125000:
            out = TWAI_TIMING_CONFIG_125KBITS();
            return true;
        case 250000:
            out = TWAI_TIMING_CONFIG_250KBITS();
            return true;
        case 500000:
            out = TWAI_TIMING_CONFIG_500KBITS();
            return true;
        case 800000:
            out = TWAI_TIMING_CONFIG_800KBITS();
            return true;
        case 1000000:
            out = TWAI_TIMING_CONFIG_1MBITS();
            return true;
        default:
            return false;
    }
}

esp_err_t Can::begin(const Config& conf) {
    end();
    twai_timing_config_t timing;
    if (!timingFor(conf.bitrate, timing)) return ESP_ERR_INVALID_ARG;

    twai_mode_t mode = TWAI_MODE_NORMAL;
    if (conf.mode == Mode::ListenOnly) mode = TWAI_MODE_LISTEN_ONLY;
    if (conf.mode == Mode::NoAck) mode = TWAI_MODE_NO_ACK;
    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)conf.tx, (gpio_num_t)conf.rx, mode);
    general.rx_queue_len = conf.rxQueue;
    general.tx_queue_len = conf.txQueue;

    twai_filter_config_t filter;
    filter.acceptance_code = conf.filterCode;
    filter.acceptance_mask = conf.filterMask;
    filter.single_filter = conf.singleFilter;

    esp_err_t err = twai_driver_install(&general, &timing, &filter);
    if (err != ESP_OK) return err;
    err = twai_start();
    if (err != ESP_OK) {
        twai_driver_uninstall();
        return err;
    }
    running = true;
    return ESP_OK;
}

void Can::end() {
    if (!running) return;
    twai_stop();
    twai_driver_uninstall();
    running = false;
}

esp_err_t Can::send(const twai_message_t& msg, uint32_t timeoutMs) {
    if (!running) return ESP_ERR_INVALID_STATE;
    return twai_transmit(&msg, pdMS_TO_TICKS(timeoutMs));
}

esp_err_t Can::receive(twai_message_t& msg, uint32_t timeoutMs) {
    if (!running) return ESP_ERR_INVALID_STATE;
    return twai_receive(&msg, pdMS_TO_TICKS(timeoutMs));
}

esp_err_t Can::status(twai_status_info_t& info) {
    if (!running) return ESP_ERR_INVALID_STATE;
    return twai_get_status_info(&info);
}

esp_err_t Can::recover() {
    twai_status_info_t info;
    esp_err_t err = status(info);
    if (err != ESP_OK) return err;
    if (info.state == TWAI_STATE_BUS_OFF) return twai_initiate_recovery();
    if (info.state == TWAI_STATE_STOPPED) return twai_start();
    return ESP_OK;
}

} // namespace mod
} // namespace lilka
