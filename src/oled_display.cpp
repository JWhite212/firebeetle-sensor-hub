// oled_display.cpp — 128x32 SSD1306 OLED with rotating sensor pages
#include "oled_display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "pins.h"
#include "version.h"
#include "timer_utils.h"
#include "sensor_hub.h"
#include "wifi_manager.h"

namespace oled_display {

namespace {
    constexpr int SCREEN_W = 128;
    constexpr int SCREEN_H = 32;

    Adafruit_SSD1306 _display(SCREEN_W, SCREEN_H, &Wire, -1);
    Timer _page_timer(OLED_PAGE_INTERVAL_MS);
    bool _ok = false;
    uint8_t _page = 0;
    constexpr uint8_t PAGE_COUNT = 4;

    /// Helper: print a value line in large font — fits 128x32 well
    void draw_large_value(const char* label, float value, const char* unit, int decimals = 1) {
        _display.clearDisplay();

        // Label in small font at top-left
        _display.setTextSize(1);
        _display.setCursor(0, 0);
        _display.print(label);

        // Value in large font, centred vertically
        _display.setTextSize(2);
        _display.setCursor(0, 16);
        _display.print(value, decimals);
        _display.setTextSize(1);
        _display.setCursor(_display.getCursorX() + 2, 16);
        _display.print(unit);

        _display.display();
    }

    /// Helper: two sensor values stacked (for dual-line pages)
    void draw_two_values(const char* label1, float v1, const char* u1,
                         const char* label2, float v2, const char* u2) {
        _display.clearDisplay();
        _display.setTextSize(1);

        // Line 1: top half
        _display.setCursor(0, 0);
        _display.printf("%s: ", label1);
        _display.print(v1, 1);
        _display.printf(" %s", u1);

        // Line 2: bottom half
        _display.setCursor(0, 16);
        _display.printf("%s: ", label2);
        _display.print(v2, 1);
        _display.printf(" %s", u2);

        _display.display();
    }

    /// Helper: three values stacked (compact — 8px per line leaves room)
    void draw_three_values(const char* l1, float v1, const char* u1,
                           const char* l2, float v2, const char* u2,
                           const char* l3, float v3, const char* u3) {
        _display.clearDisplay();
        _display.setTextSize(1);

        _display.setCursor(0, 0);
        _display.printf("%s: %.1f %s", l1, v1, u1);

        _display.setCursor(0, 11);
        _display.printf("%s: %.1f %s", l2, v2, u2);

        _display.setCursor(0, 22);
        _display.printf("%s: %.1f %s", l3, v3, u3);

        _display.display();
    }

    void render_page() {
        switch (_page) {
            case 0: {
                // Page 0: DHT22 primary — temp + humidity
                auto t = sensor_hub::get_latest("dht22_temperature");
                auto h = sensor_hub::get_latest("dht22_humidity");
                if (t.valid && h.valid) {
                    draw_two_values("Temp", t.value, "\xF8""C",
                                    "Hum", h.value, "%");
                } else {
                    _display.clearDisplay();
                    _display.setTextSize(1);
                    _display.setCursor(0, 12);
                    _display.print("DHT22: waiting...");
                    _display.display();
                }
                break;
            }
            case 1: {
                // Page 1: BH1750 lux — large display
                auto lx = sensor_hub::get_latest("illuminance");
                if (lx.valid) {
                    draw_large_value("Light", lx.value, "lx", 0);
                } else {
                    _display.clearDisplay();
                    _display.setTextSize(1);
                    _display.setCursor(0, 12);
                    _display.print("BH1750: waiting...");
                    _display.display();
                }
                break;
            }
            case 2: {
                // Page 2: DHT11-A — temp + humidity
                auto t = sensor_hub::get_latest("dht11a_temperature");
                auto h = sensor_hub::get_latest("dht11a_humidity");
                if (t.valid && h.valid) {
                    draw_two_values("11-A T", t.value, "\xF8""C",
                                    "11-A H", h.value, "%");
                } else {
                    _display.clearDisplay();
                    _display.setTextSize(1);
                    _display.setCursor(0, 12);
                    _display.print("DHT11-A: waiting...");
                    _display.display();
                }
                break;
            }
            case 3: {
                // Page 3: DHT11-B — temp + humidity
                auto t = sensor_hub::get_latest("dht11b_temperature");
                auto h = sensor_hub::get_latest("dht11b_humidity");
                if (t.valid && h.valid) {
                    draw_two_values("11-B T", t.value, "\xF8""C",
                                    "11-B H", h.value, "%");
                } else {
                    _display.clearDisplay();
                    _display.setTextSize(1);
                    _display.setCursor(0, 12);
                    _display.print("DHT11-B: waiting...");
                    _display.display();
                }
                break;
            }
        }
    }
}

void init() {
    if (_display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR_OLED)) {
        _ok = true;
        _display.clearDisplay();
        _display.setTextColor(SSD1306_WHITE);
        _display.setTextSize(1);
        _display.setCursor(0, 4);
        _display.println("Sensor Hub");
        _display.setCursor(0, 16);
        _display.printf("v%s booting...", FIRMWARE_VERSION);
        _display.display();
        Serial.println("[OLED] Display initialised (128x32)");
    } else {
        Serial.println("[OLED] SSD1306 init FAILED");
    }
}

void update() {
    if (!_ok) return;

    if (_page_timer.ready()) {
        render_page();
        _page = (_page + 1) % PAGE_COUNT;
    }
}

bool is_ready() {
    return _ok;
}

} // namespace oled_display
