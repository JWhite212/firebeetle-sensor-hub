// main.cpp — Firebeetle ESP32-E Multi-Sensor Hub
// Entry point: init modules, then dispatch non-blocking updates in loop().
// No business logic belongs here — everything lives in its module.
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "pins.h"
#include "version.h"
#include "timer_utils.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "ota_handler.h"
#include "sensor_hub.h"
#include "oled_display.h"

// Sensor drivers
#include "sensors/dht_sensor.h"
#include "sensors/bh1750_sensor.h"

// ── Shared DHT hardware handles ─────────────────────
// Each physical DHT chip is shared between its temp + humidity sensor objects.
static auto dht22_device  = std::make_shared<DHTDevice>(PIN_DHT22,   DHT22);
static auto dht11a_device = std::make_shared<DHTDevice>(PIN_DHT11_A, DHT11);
static auto dht11b_device = std::make_shared<DHTDevice>(PIN_DHT11_B, DHT11);

// ── MQTT publish timer ──────────────────────────────
static Timer publish_timer(MQTT_PUBLISH_INTERVAL_MS);

/// Build and publish the combined JSON state payload for all sensors.
void publish_sensor_state() {
    if (!mqtt_client::is_ready()) return;

    JsonDocument doc;

    // DHT22 primary
    auto r = sensor_hub::get_latest("dht22_temperature");
    if (r.valid) doc["dht22_temperature"] = round(r.value * 10.0f) / 10.0f;
    r = sensor_hub::get_latest("dht22_humidity");
    if (r.valid) doc["dht22_humidity"] = round(r.value * 10.0f) / 10.0f;

    // DHT11 A
    r = sensor_hub::get_latest("dht11a_temperature");
    if (r.valid) doc["dht11a_temperature"] = round(r.value * 10.0f) / 10.0f;
    r = sensor_hub::get_latest("dht11a_humidity");
    if (r.valid) doc["dht11a_humidity"] = round(r.value * 10.0f) / 10.0f;

    // DHT11 B
    r = sensor_hub::get_latest("dht11b_temperature");
    if (r.valid) doc["dht11b_temperature"] = round(r.value * 10.0f) / 10.0f;
    r = sensor_hub::get_latest("dht11b_humidity");
    if (r.valid) doc["dht11b_humidity"] = round(r.value * 10.0f) / 10.0f;

    // BH1750
    r = sensor_hub::get_latest("illuminance");
    if (r.valid) doc["illuminance"] = round(r.value * 10.0f) / 10.0f;

    // WiFi RSSI (diagnostic)
    doc["wifi_rssi"] = wifi_manager::get_rssi();

    char buf[384];
    serializeJson(doc, buf, sizeof(buf));
    mqtt_client::publish_state(buf);

    Serial.printf("[Main] Published state: %s\n", buf);
}

void setup() {
    Serial.begin(115200);
    delay(100);  // Only acceptable delay() — let serial settle

    Serial.printf("\n╔══════════════════════════════════════╗\n");
    Serial.printf("║  %s v%s       ║\n", DEVICE_FRIENDLY_NAME, FIRMWARE_VERSION);
    Serial.printf("║  Chip: %08X                      ║\n", (uint32_t)ESP.getEfuseMac());
    Serial.printf("╚══════════════════════════════════════╝\n\n");

    // ── Watchdog ────────────────────────────────────
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
        esp_task_wdt_config_t wdt_cfg = {
            .timeout_ms = WDT_TIMEOUT_S * 1000,
            .idle_core_mask = 0,
            .trigger_panic = true
        };
        esp_task_wdt_reconfigure(&wdt_cfg);
    #else
        esp_task_wdt_init(WDT_TIMEOUT_S, true);
    #endif
    esp_task_wdt_add(NULL);

    // ── I2C bus (shared by BH1750 + OLED) ──────────
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // ── I2C scan (diagnostic) ────────────────────────
    Serial.println("[I2C] Scanning bus...");
    uint8_t devices_found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] Device found at 0x%02X\n", addr);
            devices_found++;
        }
    }
    Serial.printf("[I2C] Scan complete — %u device(s) found\n\n", devices_found);

    // ── Register sensors ───────────────────────────
    // DHT22 — primary (higher accuracy: ±0.5°C, ±2% RH)
    sensor_hub::add_sensor(std::make_unique<DHTTempSensor>(dht22_device, "dht22_temperature"));
    sensor_hub::add_sensor(std::make_unique<DHTHumiditySensor>(dht22_device, "dht22_humidity"));

    // DHT11 A — secondary (±2°C, ±5% RH)
    sensor_hub::add_sensor(std::make_unique<DHTTempSensor>(dht11a_device, "dht11a_temperature"));
    sensor_hub::add_sensor(std::make_unique<DHTHumiditySensor>(dht11a_device, "dht11a_humidity"));

    // DHT11 B — secondary
    sensor_hub::add_sensor(std::make_unique<DHTTempSensor>(dht11b_device, "dht11b_temperature"));
    sensor_hub::add_sensor(std::make_unique<DHTHumiditySensor>(dht11b_device, "dht11b_humidity"));

    // BH1750 — I2C lux sensor
    sensor_hub::add_sensor(std::make_unique<BH1750Sensor>(I2C_ADDR_BH1750));

    // ── Initialise all modules ─────────────────────
    wifi_manager::init();
    mqtt_client::init();
    ota_handler::init();
    sensor_hub::init();
    oled_display::init();

    Serial.printf("[Main] %zu sensors registered\n", sensor_hub::count());
    Serial.println("[Main] Setup complete — entering main loop\n");
}

void loop() {
    esp_task_wdt_reset();

    wifi_manager::update();
    mqtt_client::update();
    ota_handler::update();
    sensor_hub::update();
    oled_display::update();

    // Publish combined state on schedule
    if (publish_timer.ready()) {
        publish_sensor_state();
    }

    yield();
}
