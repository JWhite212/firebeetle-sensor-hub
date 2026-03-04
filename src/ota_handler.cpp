// ota_handler.cpp
#include "ota_handler.h"
#include <ArduinoOTA.h>
#include "config.h"
#include "wifi_manager.h"

namespace ota_handler {

namespace {
    bool _initialised = false;
}

void init() {
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH)
                          ? "firmware" : "filesystem";
        Serial.printf("[OTA] Start updating %s\n", type.c_str());
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Complete — rebooting");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:    Serial.println("Auth failed"); break;
            case OTA_BEGIN_ERROR:   Serial.println("Begin failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("Connect failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive failed"); break;
            case OTA_END_ERROR:     Serial.println("End failed"); break;
        }
    });

    // OTA.begin() requires WiFi — defer until connected
    _initialised = false;
}

void update() {
    if (!wifi_manager::is_ready()) return;

    if (!_initialised) {
        ArduinoOTA.begin();
        _initialised = true;
        Serial.printf("[OTA] Ready at %s\n", wifi_manager::get_ip());
    }

    ArduinoOTA.handle();
}

} // namespace ota_handler
