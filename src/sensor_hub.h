// sensor_hub.h
#pragma once
#include "sensors/base_sensor.h"
#include <memory>
#include <vector>

namespace sensor_hub {
    /// Register a sensor. The hub takes ownership.
    void add_sensor(std::unique_ptr<BaseSensor> sensor);

    /// Initialise all registered sensors. Call once in setup().
    void init();

    /// Check schedules and read sensors when due. Call every loop().
    void update();

    /// Get the latest reading for a named sensor. Returns invalid if not found.
    SensorReading get_latest(const char* sensor_name);

    /// Get all latest readings (for bulk MQTT publish / OLED display).
    struct SensorSnapshot {
        const char* name;
        float value;
        const char* unit;
        bool valid;
    };
    const std::vector<SensorSnapshot>& get_all_snapshots();

    /// Returns true when at least one sensor is operational.
    bool is_ready();

    /// How many sensors are registered.
    size_t count();
}
