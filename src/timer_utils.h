// timer_utils.h — Non-blocking millis()-based timer
#pragma once
#include <Arduino.h>

/// Non-blocking timer. Handles millis() rollover correctly via unsigned subtraction.
class Timer {
public:
    explicit Timer(uint32_t interval_ms)
        : _interval(interval_ms), _last(0), _started(false) {}

    /// Returns true once per interval period.
    bool ready() {
        uint32_t now = millis();
        if (!_started) {
            _last = now;
            _started = true;
            return false;
        }
        if (now - _last >= _interval) {
            _last = now;
            return true;
        }
        return false;
    }

    /// Force the timer to fire on next ready() call.
    void trigger() { _last = millis() - _interval; }

    /// Reset without firing.
    void reset() { _last = millis(); }

    /// Change interval at runtime.
    void set_interval(uint32_t interval_ms) { _interval = interval_ms; }

private:
    uint32_t _interval;
    uint32_t _last;
    bool     _started;
};
