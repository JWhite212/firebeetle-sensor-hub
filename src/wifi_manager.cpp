// wifi_manager.cpp
#include "wifi_manager.h"
#include <WiFi.h>
#include "config.h"
#include "pins.h"
#include "timer_utils.h"

namespace wifi_manager {

namespace {
    enum class State { DISCONNECTED, CONNECTING, CONNECTED };
    State _state = State::DISCONNECTED;

    uint32_t _backoff_ms = 1000;
    constexpr uint32_t BACKOFF_MAX_MS = 30000;

    Timer _reconnect_timer(1000);
    char  _ip_str[16] = "";

    void on_wifi_event(WiFiEvent_t event) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                _state = State::CONNECTED;
                _backoff_ms = 1000;
                snprintf(_ip_str, sizeof(_ip_str), "%s",
                         WiFi.localIP().toString().c_str());
                Serial.printf("[WiFi] Connected — IP: %s (RSSI: %d dBm)\n",
                              _ip_str, WiFi.RSSI());
                break;

            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                if (_state == State::CONNECTED) {
                    Serial.println("[WiFi] Disconnected — will reconnect");
                }
                _state = State::DISCONNECTED;
                _ip_str[0] = '\0';
                break;

            default:
                break;
        }
    }

    void start_connection() {
        Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        _state = State::CONNECTING;
    }
}

void init() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);

    // Unique hostname for mDNS
    char hostname[48];
    snprintf(hostname, sizeof(hostname), "%s-%08X",
             DEVICE_NAME, (uint32_t)ESP.getEfuseMac());
    WiFi.setHostname(hostname);

    WiFi.onEvent(on_wifi_event);
    start_connection();
}

void update() {
    if (_state == State::DISCONNECTED && _reconnect_timer.ready()) {
        Serial.printf("[WiFi] Reconnecting (backoff: %lums)\n", _backoff_ms);
        start_connection();
        _backoff_ms = min(_backoff_ms * 2, BACKOFF_MAX_MS);
        _reconnect_timer.set_interval(_backoff_ms);
    }
}

bool is_ready() {
    return _state == State::CONNECTED && WiFi.isConnected();
}

const char* get_ip() {
    return _ip_str;
}

int get_rssi() {
    return WiFi.RSSI();
}

} // namespace wifi_manager
