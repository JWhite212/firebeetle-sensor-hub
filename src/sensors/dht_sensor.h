// sensors/dht_sensor.h — DHT22/DHT11 temperature and humidity drivers
// Each physical DHT sensor produces TWO BaseSensor instances (temp + humidity).
// The DHT library object is shared between them to avoid double-init on the same pin.
#pragma once
#include "base_sensor.h"
#include <DHT.h>
#include <memory>

/// Shared DHT hardware handle. Both temp and humidity sensors reference this.
struct DHTDevice {
    DHT dht;
    bool initialised = false;

    DHTDevice(int pin, uint8_t type) : dht(pin, type) {}

    bool begin() {
        if (!initialised) {
            dht.begin();
            initialised = true;
        }
        return true;
    }
};

/// DHT temperature sensor.
class DHTTempSensor : public BaseSensor {
public:
    DHTTempSensor(std::shared_ptr<DHTDevice> dev, const char* name)
        : _dev(dev), _name(name) {}

    const char* name() const override { return _name; }
    bool begin() override { return _dev->begin(); }
    uint32_t min_interval_ms() const override { return 2500; }  // DHT needs ~2s min
    float range_min() const override { return -40.0f; }
    float range_max() const override { return 80.0f; }

    SensorReading read() override {
        float t = _dev->dht.readTemperature();
        return validate(t, _name, "°C");
    }

private:
    std::shared_ptr<DHTDevice> _dev;
    const char* _name;
};

/// DHT humidity sensor.
class DHTHumiditySensor : public BaseSensor {
public:
    DHTHumiditySensor(std::shared_ptr<DHTDevice> dev, const char* name)
        : _dev(dev), _name(name) {}

    const char* name() const override { return _name; }
    bool begin() override { return _dev->begin(); }
    uint32_t min_interval_ms() const override { return 2500; }
    float range_min() const override { return 0.0f; }
    float range_max() const override { return 100.0f; }

    SensorReading read() override {
        float h = _dev->dht.readHumidity();
        return validate(h, _name, "%");
    }

private:
    std::shared_ptr<DHTDevice> _dev;
    const char* _name;
};
