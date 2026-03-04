// mqtt_client.h — MQTT client with HA auto-discovery
#pragma once
#include <functional>

namespace mqtt_client {
    /// Connect to the MQTT broker. Requires wifi_manager to be initialised.
    void init();

    /// Handle reconnection and incoming messages. Call every loop().
    void update();

    /// Returns true when connected to the broker.
    bool is_ready();

    /// Publish a message to a subtopic under the device base topic.
    bool publish(const char* subtopic, const char* payload, bool retained = false);

    /// Publish the combined sensor state JSON payload.
    bool publish_state(const char* json_payload);

    /// Publish HA MQTT auto-discovery configs for all sensors.
    void publish_discovery();

    /// Subscribe to a subtopic with a callback.
    using MessageCallback = std::function<void(const char* topic, const char* payload)>;
    void subscribe(const char* subtopic, MessageCallback callback);
}
