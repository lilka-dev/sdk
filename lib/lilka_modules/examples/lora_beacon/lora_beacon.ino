// Sends a LoRa beacon every 10 s and prints received packets (003 LORA module, E22-900M).
#include <lilka.h>
#include <lilka_modules.h>

using namespace lilka::mod;

SX126x radio;

void setup() {
    lilka::begin();
    lilka::SPI2.begin(pins::SPI_SCK, pins::SPI_MISO, pins::SPI_MOSI);
    SX126x::Config cfg;
    cfg.spi = &lilka::SPI2;
    cfg.frequencyHz = 868100000; // check what is allowed where you are
    cfg.powerDbm = 0;
    if (radio.begin(cfg) != SX126x::Result::OK) {
        Serial.println("No SX126x found");
        return;
    }
    radio.startReceive();
}

void loop() {
    static uint32_t lastTx = 0;
    static uint32_t counter = 0;
    SX126x::Packet packet;
    if (radio.receive(packet)) {
        Serial.printf("RX %.*s rssi=%.1f snr=%.1f\n", packet.length, (const char*)packet.data, packet.rssi, packet.snr);
    }
    if (millis() - lastTx > 10000) {
        char msg[32];
        int n = snprintf(msg, sizeof(msg), "LILKA #%lu", (unsigned long)counter++);
        radio.send(reinterpret_cast<const uint8_t*>(msg), n);
        lastTx = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
}
