#include "gps.h"

namespace lilka {
namespace mod {

Gps::~Gps() {
    end();
}

esp_err_t Gps::begin(const Config& conf) {
    end();
    cfg = conf;
    uart_config_t uc = {};
    uc.baud_rate = (int)cfg.baud;
    uc.data_bits = UART_DATA_8_BITS;
    uc.parity = UART_PARITY_DISABLE;
    uc.stop_bits = UART_STOP_BITS_1;
    uc.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uc.source_clk = UART_SCLK_APB;
    esp_err_t err = uart_driver_install(cfg.port, 2048, 0, 0, nullptr, 0);
    if (err != ESP_OK) return err;
    err = uart_param_config(cfg.port, &uc);
    if (err == ESP_OK) {
        err = uart_set_pin(
            cfg.port, cfg.tx < 0 ? UART_PIN_NO_CHANGE : cfg.tx, cfg.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE
        );
    }
    if (err != ESP_OK) {
        uart_driver_delete(cfg.port);
        return err;
    }
    parser.reset();
    running = true;
    return ESP_OK;
}

void Gps::end() {
    if (!running) return;
    uart_driver_delete(cfg.port);
    running = false;
}

int Gps::update() {
    if (!running) return 0;
    int sentences = 0;
    uint8_t chunk[128];
    for (;;) {
        int n = uart_read_bytes(cfg.port, chunk, sizeof(chunk), 0);
        if (n <= 0) break;
        for (int i = 0; i < n; i++) {
            if (parser.feed((char)chunk[i])) sentences++;
        }
    }
    return sentences;
}

esp_err_t Gps::write(const uint8_t* data, size_t len) {
    if (!running) return ESP_ERR_INVALID_STATE;
    if (cfg.tx < 0) return ESP_ERR_NOT_SUPPORTED;
    return uart_write_bytes(cfg.port, data, len) == (int)len ? ESP_OK : ESP_FAIL;
}

esp_err_t Gps::sendUbx(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t len) {
    uint8_t head[6] = {0xB5, 0x62, cls, id, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)};
    uint8_t ckA = 0, ckB = 0;
    for (int i = 2; i < 6; i++) {
        ckA += head[i];
        ckB += ckA;
    }
    for (uint16_t i = 0; i < len; i++) {
        ckA += payload[i];
        ckB += ckA;
    }
    esp_err_t err = write(head, sizeof(head));
    if (err == ESP_OK && len) err = write(payload, len);
    if (err == ESP_OK) {
        const uint8_t tail[2] = {ckA, ckB};
        err = write(tail, sizeof(tail));
    }
    return err;
}

} // namespace mod
} // namespace lilka
