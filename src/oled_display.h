// oled_display.h — SSD1306 128x32 OLED display driver
#pragma once

namespace oled_display {
    /// Initialise the SSD1306 over I2C. Call after Wire.begin().
    void init();

    /// Cycle through display pages on schedule. Call every loop().
    void update();

    /// Returns true if the display initialised successfully.
    bool is_ready();
}
