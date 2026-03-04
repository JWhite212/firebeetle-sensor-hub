// mqtt_client.cpp — MQTT with Home Assistant auto-discovery
#include "mqtt_client.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "version.h"
#include "timer_utils.h"
#include "wifi_manager.h"

namespace mqtt_client {

namespace {
    WiFiClient   _wifi_client;
    PubSubClient _mqtt(_wifi_client);

    Timer _reconnect_timer(5000);
    Timer _discovery_timer(DISCOVERY_INTERVAL_MS);
    uint32_t _backoff_ms = 2000;
    constexpr uint32_t BACKOFF_MAX_MS = 30000;

    // ── Topic buffers ───────────────────────────────────
    char _base_topic[80];    // home/<device_name>
    char _status_topic[96];  // home/<device_name>/status
    char _state_topic[96];   // home/<device_name>/state
    char _topic_buf[128];

    const char* build_topic(const char* subtopic) {
        snprintf(_topic_buf, sizeof(_topic_buf), "%s/%s",
                 _base_topic, subtopic);
        return _topic_buf;
    }

    // ── Message queue (while disconnected) ──────────────
    struct QueuedMsg {
        char topic[128];
        char payload[512];
        bool retained;
    };
    std::vector<QueuedMsg> _queue;
    constexpr size_t QUEUE_MAX = 20;

    void flush_queue() {
        for (auto& msg : _queue) {
            _mqtt.publish(msg.topic, msg.payload, msg.retained);
        }
        _queue.clear();
    }

    // ── Subscription dispatch ───────────────────────────
    struct Subscription {
        char topic[128];
        MessageCallback callback;
    };
    std::vector<Subscription> _subscriptions;

    void on_message(char* topic, byte* payload, unsigned int length) {
        char buf[512];
        size_t n = min((unsigned int)(sizeof(buf) - 1), length);
        memcpy(buf, payload, n);
        buf[n] = '\0';

        for (auto& sub : _subscriptions) {
            if (strstr(topic, sub.topic) != nullptr) {
                sub.callback(topic, buf);
            }
        }
    }

    // ── HA discovery payload builder ────────────────────
    // Publishes a retained JSON config to:
    //   homeassistant/<component>/<device_id>/<object_id>/config
    void publish_single_discovery(
        const char* component,     // "sensor" or "binary_sensor"
        const char* object_id,     // e.g. "dht22_temperature"
        const char* friendly_name, // e.g. "DHT22 Temperature"
        const char* device_class,  // e.g. "temperature" (nullable)
        const char* unit,          // e.g. "°C" (nullable)
        const char* value_tpl,     // e.g. "{{ value_json.dht22_temperature }}"
        const char* icon           // e.g. "mdi:thermometer" (nullable)
    ) {
        char topic[160];
        snprintf(topic, sizeof(topic),
                 "homeassistant/%s/%s/%s/config",
                 component, DEVICE_ID, object_id);

        JsonDocument doc;
        doc["name"] = friendly_name;

        // Unique ID: device_id + object_id
        char uid[96];
        snprintf(uid, sizeof(uid), "%s_%s", DEVICE_ID, object_id);
        doc["unique_id"] = uid;

        doc["state_topic"] = _state_topic;

        if (device_class && strlen(device_class) > 0) {
            doc["device_class"] = device_class;
        }
        if (unit && strlen(unit) > 0) {
            doc["unit_of_measurement"] = unit;
        }
        if (value_tpl && strlen(value_tpl) > 0) {
            doc["value_template"] = value_tpl;
        }
        if (icon && strlen(icon) > 0) {
            doc["icon"] = icon;
        }

        doc["state_class"] = "measurement";

        // Availability via LWT
        JsonArray avail = doc["availability"].to<JsonArray>();
        JsonObject av = avail.add<JsonObject>();
        av["topic"] = _status_topic;
        av["payload_available"] = "online";
        av["payload_not_available"] = "offline";

        // Device grouping — all entities under one device card
        JsonObject dev = doc["device"].to<JsonObject>();
        dev["identifiers"].to<JsonArray>().add(DEVICE_ID);
        dev["name"] = DEVICE_FRIENDLY_NAME;
        dev["manufacturer"] = "DIY";
        dev["model"] = "Firebeetle ESP32-E Multi-Sensor";
        dev["sw_version"] = FIRMWARE_VERSION;

        char payload[768];
        serializeJson(doc, payload, sizeof(payload));
        _mqtt.publish(topic, payload, true);

        Serial.printf("[MQTT] Discovery: %s → %s\n", object_id, topic);
    }

    void attempt_connect() {
        char client_id[48];
        snprintf(client_id, sizeof(client_id), "%s-%08X",
                 MQTT_CLIENT_PREFIX, (uint32_t)ESP.getEfuseMac());

        Serial.printf("[MQTT] Connecting as %s to %s:%d...\n",
                      client_id, MQTT_BROKER, MQTT_PORT);

        bool connected;
        if (strlen(MQTT_USER) > 0) {
            connected = _mqtt.connect(client_id, MQTT_USER, MQTT_PASSWORD,
                                      _status_topic, 1, true, "offline");
        } else {
            connected = _mqtt.connect(client_id, nullptr, nullptr,
                                      _status_topic, 1, true, "offline");
        }

        if (connected) {
            Serial.println("[MQTT] Connected");
            _backoff_ms = 2000;

            // Publish birth message
            _mqtt.publish(_status_topic, "online", true);

            // Re-subscribe
            for (auto& sub : _subscriptions) {
                _mqtt.subscribe(sub.topic);
            }

            // Publish discovery immediately on connect
            publish_discovery();

            flush_queue();
        } else {
            Serial.printf("[MQTT] Failed, rc=%d\n", _mqtt.state());
            _backoff_ms = min(_backoff_ms * 2, BACKOFF_MAX_MS);
            _reconnect_timer.set_interval(_backoff_ms);
        }
    }
}

void init() {
    // Topic hierarchy: home/<device_name>/...
    snprintf(_base_topic, sizeof(_base_topic), "home/%s", DEVICE_NAME);
    snprintf(_status_topic, sizeof(_status_topic), "%s/status", _base_topic);
    snprintf(_state_topic, sizeof(_state_topic), "%s/state", _base_topic);

    _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    _mqtt.setCallback(on_message);
    _mqtt.setBufferSize(1024);
    _mqtt.setKeepAlive(60);
}

void update() {
    if (!wifi_manager::is_ready()) return;

    if (!_mqtt.connected()) {
        if (_reconnect_timer.ready()) {
            attempt_connect();
        }
        return;
    }

    _mqtt.loop();

    // Re-publish discovery periodically (handles HA restarts)
    if (_discovery_timer.ready()) {
        publish_discovery();
    }
}

bool is_ready() {
    return _mqtt.connected();
}

bool publish(const char* subtopic, const char* payload, bool retained) {
    const char* full_topic = build_topic(subtopic);

    if (_mqtt.connected()) {
        return _mqtt.publish(full_topic, payload, retained);
    }

    if (_queue.size() < QUEUE_MAX) {
        QueuedMsg msg;
        strncpy(msg.topic, full_topic, sizeof(msg.topic) - 1);
        msg.topic[sizeof(msg.topic) - 1] = '\0';
        strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        msg.retained = retained;
        _queue.push_back(msg);
        return true;
    }

    Serial.println("[MQTT] Queue full — dropping message");
    return false;
}

bool publish_state(const char* json_payload) {
    if (_mqtt.connected()) {
        return _mqtt.publish(_state_topic, json_payload, false);
    }

    if (_queue.size() < QUEUE_MAX) {
        QueuedMsg msg;
        strncpy(msg.topic, _state_topic, sizeof(msg.topic) - 1);
        msg.topic[sizeof(msg.topic) - 1] = '\0';
        strncpy(msg.payload, json_payload, sizeof(msg.payload) - 1);
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        msg.retained = false;
        _queue.push_back(msg);
        return true;
    }
    return false;
}

void publish_discovery() {
    // ── DHT22 (primary) ─────────────────────────────────
    publish_single_discovery(
        "sensor", "dht22_temperature", "DHT22 Temperature",
        "temperature", "°C",
        "{{ value_json.dht22_temperature }}", nullptr);
    publish_single_discovery(
        "sensor", "dht22_humidity", "DHT22 Humidity",
        "humidity", "%",
        "{{ value_json.dht22_humidity }}", nullptr);

    // ── DHT11 A (secondary) ─────────────────────────────
    publish_single_discovery(
        "sensor", "dht11a_temperature", "DHT11-A Temperature",
        "temperature", "°C",
        "{{ value_json.dht11a_temperature }}", nullptr);
    publish_single_discovery(
        "sensor", "dht11a_humidity", "DHT11-A Humidity",
        "humidity", "%",
        "{{ value_json.dht11a_humidity }}", nullptr);

    // ── DHT11 B (secondary) ─────────────────────────────
    publish_single_discovery(
        "sensor", "dht11b_temperature", "DHT11-B Temperature",
        "temperature", "°C",
        "{{ value_json.dht11b_temperature }}", nullptr);
    publish_single_discovery(
        "sensor", "dht11b_humidity", "DHT11-B Humidity",
        "humidity", "%",
        "{{ value_json.dht11b_humidity }}", nullptr);

    // ── BH1750 ──────────────────────────────────────────
    publish_single_discovery(
        "sensor", "illuminance", "Illuminance",
        "illuminance", "lx",
        "{{ value_json.illuminance }}", nullptr);

    // ── WiFi signal (diagnostic) ────────────────────────
    publish_single_discovery(
        "sensor", "wifi_rssi", "WiFi Signal",
        "signal_strength", "dBm",
        "{{ value_json.wifi_rssi }}", "mdi:wifi");

    Serial.println("[MQTT] All discovery configs published");
}

void subscribe(const char* subtopic, MessageCallback callback) {
    Subscription sub;
    snprintf(sub.topic, sizeof(sub.topic), "%s/%s", _base_topic, subtopic);
    sub.callback = callback;
    _subscriptions.push_back(sub);

    if (_mqtt.connected()) {
        _mqtt.subscribe(sub.topic);
    }
}

} // namespace mqtt_client
