// pins.h — DFRobot Firebeetle ESP32-E pin assignments
#pragma once

// ── Board: DFRobot Firebeetle 2 ESP32-E ─────────────
// Ref: https://wiki.dfrobot.com/FireBeetle_Board_ESP32_E_SKU_DFR0654
//
// Firebeetle silkscreen → GPIO mapping used below:
//   D2=GPIO25, D3=GPIO26, D6=GPIO14, D7=GPIO27, D9=GPIO5
//   SDA=GPIO21, SCL=GPIO22

// Onboard LED
constexpr int PIN_STATUS_LED = 2;

// ── I2C bus (BH1750 + SSD1306 OLED share this bus) ──
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// ── DHT sensors (each needs a 4.7kΩ pull-up to 3.3V) ─
constexpr int PIN_DHT22   = 13;   // D6 — primary temp/humidity
constexpr int PIN_DHT11_A = 14;   // D7 — secondary sensor A
constexpr int PIN_DHT11_B = 0;   // D3 — secondary sensor B

// ── I2C device addresses ────────────────────────────
constexpr uint8_t I2C_ADDR_BH1750 = 0x23;
constexpr uint8_t I2C_ADDR_OLED   = 0x3C;  // 0.91" SSD1306 128x32
