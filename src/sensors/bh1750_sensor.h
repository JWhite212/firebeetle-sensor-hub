// sensors/bh1750_sensor.h — BH1750 ambient light sensor (I2C)
#pragma once
#include "base_sensor.h"
#include <BH1750.h>
#include <Wire.h>

class BH1750Sensor : public BaseSensor {
public:
    BH1750Sensor(uint8_t addr = 0x23) : _addr(addr) {}

    const char* name() const override { return "illuminance"; }

    bool begin() override {
        return _bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, _addr);
    }

    uint32_t min_interval_ms() const override { return 1000; }
    float range_min() const override { return 0.0f; }
    float range_max() const override { return 65535.0f; }

    SensorReading read() override {
        if (!_bh.measurementReady()) {
            return { 0, "illuminance", "lx", false, millis() };
        }
        float lux = _bh.readLightLevel();
        return validate(lux, "illuminance", "lx");
    }

private:
    BH1750 _bh;
    uint8_t _addr;
};
