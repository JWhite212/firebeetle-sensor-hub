// sensors/base_sensor.h — Abstract interface for all sensors
#pragma once
#include <Arduino.h>

/// Result of a sensor read attempt.
struct SensorReading {
    float value;
    const char* name;    // Sensor name (e.g., "dht22_temperature")
    const char* unit;    // Unit string (e.g., "°C")
    bool valid;          // false if read failed or value out of range
    uint32_t timestamp;  // millis() at read time
};

/// Abstract base class all sensor drivers inherit from.
class BaseSensor {
public:
    virtual ~BaseSensor() = default;

    /// Human-readable name used in MQTT topics and HA discovery.
    virtual const char* name() const = 0;

    /// Initialise hardware. Returns false on failure.
    virtual bool begin() = 0;

    /// Perform a read. Returns a validated SensorReading.
    virtual SensorReading read() = 0;

    /// Minimum interval between reads in ms (per datasheet).
    virtual uint32_t min_interval_ms() const { return 2000; }

    /// Valid range — reads outside are rejected.
    virtual float range_min() const { return -1e6; }
    virtual float range_max() const { return 1e6; }

protected:
    /// Validate a raw reading against this sensor's range.
    SensorReading validate(float raw, const char* sensor_name, const char* unit) {
        bool valid = !isnan(raw) && !isinf(raw)
                     && raw >= range_min() && raw <= range_max();
        return { raw, sensor_name, unit, valid, millis() };
    }
};
