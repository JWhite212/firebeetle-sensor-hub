// sensor_hub.cpp
#include "sensor_hub.h"
#include "config.h"
#include "timer_utils.h"
#include <algorithm>

namespace sensor_hub {

namespace {
    struct SensorEntry {
        std::unique_ptr<BaseSensor> sensor;
        Timer timer;
        SensorReading last_reading;
        bool operational;

        SensorEntry(std::unique_ptr<BaseSensor> s)
            : sensor(std::move(s)),
              timer(0),
              last_reading{0, "", "", false, 0},
              operational(false) {}
    };

    std::vector<SensorEntry> _sensors;
    std::vector<SensorSnapshot> _snapshots;
    bool _any_ready = false;
}

void add_sensor(std::unique_ptr<BaseSensor> sensor) {
    _sensors.emplace_back(std::move(sensor));
}

void init() {
    for (auto& entry : _sensors) {
        uint32_t interval = max(entry.sensor->min_interval_ms(),
                                SENSOR_READ_INTERVAL_MS);
        entry.timer.set_interval(interval);

        if (entry.sensor->begin()) {
            entry.operational = true;
            Serial.printf("[Sensor] %s initialised (interval: %lums)\n",
                          entry.sensor->name(), interval);
        } else {
            Serial.printf("[Sensor] %s FAILED to initialise\n",
                          entry.sensor->name());
        }
    }
}

void update() {
    _any_ready = false;
    for (auto& entry : _sensors) {
        if (!entry.operational) continue;
        _any_ready = true;

        if (entry.timer.ready()) {
            SensorReading reading = entry.sensor->read();
            if (reading.valid) {
                entry.last_reading = reading;
            } else {
                Serial.printf("[Sensor] %s: invalid reading\n",
                              entry.sensor->name());
            }
        }
    }
}

SensorReading get_latest(const char* sensor_name) {
    for (auto& entry : _sensors) {
        if (strcmp(entry.sensor->name(), sensor_name) == 0) {
            return entry.last_reading;
        }
    }
    return {0, sensor_name, "", false, 0};
}

const std::vector<SensorSnapshot>& get_all_snapshots() {
    _snapshots.clear();
    for (auto& entry : _sensors) {
        if (entry.operational && entry.last_reading.valid) {
            _snapshots.push_back({
                entry.last_reading.name,
                entry.last_reading.value,
                entry.last_reading.unit,
                entry.last_reading.valid
            });
        }
    }
    return _snapshots;
}

bool is_ready() {
    return _any_ready;
}

size_t count() {
    return _sensors.size();
}

} // namespace sensor_hub
