# Firebeetle ESP32-E Multi-Sensor Hub

Production-grade environmental sensor hub running on a DFRobot Firebeetle 2 ESP32-E,
publishing to Home Assistant via MQTT auto-discovery with a local 128×32 OLED display.

## Architecture

```
┌─────────────────────────────────────────────────┐
│  Firebeetle ESP32-E                             │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ DHT22    │  │ DHT11 ×2 │  │ BH1750 (I2C) │  │
│  │ GPIO 14  │  │ GPIO 27  │  │ 0x23         │  │
│  │          │  │ GPIO 26  │  │              │  │
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘  │
│       │              │               │          │
│  ┌────┴──────────────┴───────────────┴───────┐  │
│  │            Sensor Hub (scheduler)         │  │
│  └────────────────────┬──────────────────────┘  │
│                       │                         │
│  ┌────────────┐  ┌────┴───────┐  ┌───────────┐ │
│  │ OLED 128×32│  │ MQTT Client│  │ OTA       │ │
│  │ 0x3C (I2C) │  │ PubSubCli  │  │ ArduinoOTA│ │
│  └────────────┘  └────┬───────┘  └───────────┘ │
│                       │                         │
└───────────────────────┼─────────────────────────┘
                        │ WiFi
                        ▼
               ┌─────────────────┐
               │  Mosquitto MQTT │
               │  (HA Add-on)   │
               └────────┬────────┘
                        │ Auto-Discovery
                        ▼
               ┌─────────────────┐
               │  Home Assistant │
               │  (VirtualBox)  │
               └─────────────────┘
```

## Hardware

### Bill of Materials

| Component | Quantity | UK Source | Approx. Price |
|-----------|----------|-----------|---------------|
| DFRobot Firebeetle 2 ESP32-E | 1 | The Pi Hut / Amazon UK | £12–15 |
| DHT22 (AM2302) module | 1 | Amazon UK / AliExpress | £3–5 |
| DHT11 module | 2 | Amazon UK / AliExpress | £1–3 each |
| BH1750 (GY-302) breakout | 1 | Amazon UK / AliExpress | £2–4 |
| SSD1306 OLED 0.91" 128×32 | 1 | Amazon UK / AliExpress | £3–5 |
| 4.7kΩ resistors | 3 | Any | £0.10 each |
| Breadboard + jumper wires | 1 set | Any | £5–8 |
| USB-C data cable | 1 | Any | £3–5 |

### Wiring Table

```
Firebeetle ESP32-E Pinout (relevant pins):

              ┌──────────────┐
              │   USB-C      │
              │              │
     3V3  ── │ 3V3     VIN  │ ── 5V (USB power)
     GND  ── │ GND     GND  │ ── GND
              │ D7/G27  D2   │
              │ D6/G14  D3/G26│
              │ D5/G25  D4   │
              │ SDA/G21 D9   │
              │ SCL/G22 D10  │
              │              │
              └──────────────┘
```

| Firebeetle Pin | GPIO | Wire To | Notes |
|----------------|------|---------|-------|
| **SDA** | GPIO 21 | BH1750 SDA, OLED SDA | Shared I2C data bus |
| **SCL** | GPIO 22 | BH1750 SCL, OLED SCL | Shared I2C clock bus |
| **D6** | GPIO 14 | DHT22 DATA | + 4.7kΩ pull-up to 3V3 |
| **D7** | GPIO 27 | DHT11-A DATA | + 4.7kΩ pull-up to 3V3 |
| **D3** | GPIO 26 | DHT11-B DATA | + 4.7kΩ pull-up to 3V3 |
| **3V3** | — | BH1750 VCC, OLED VCC, DHT22 VCC, DHT11-A VCC, DHT11-B VCC | All sensors run at 3.3V |
| **GND** | — | ALL sensor GND pins | Common ground rail |

**Critical wiring notes:**
- Each DHT sensor needs a **4.7kΩ pull-up resistor** between DATA and 3V3.
  Many DHT breakout boards include this resistor on-board — check yours.
  If the board has a 10kΩ on-board, add an external 4.7kΩ anyway (parallel = ~3.2kΩ, fine).
- BH1750 and OLED share the I2C bus on different addresses (0x23 and 0x3C) — no conflict.
- Most BH1750 breakouts have the ADDR pin low by default → address 0x23.
  If yours reads 0x5C, the ADDR pin is high — either ground it or change `I2C_ADDR_BH1750` in `pins.h`.
- Keep wires under 30cm for reliable DHT reads. I2C works up to ~1m at 100kHz.
- The Firebeetle has an on-board voltage regulator — USB-C provides 5V, the 3V3 pin is regulated output.

### I2C Address Map

| Device | Address | Function |
|--------|---------|----------|
| BH1750 | 0x23 | Ambient light (lux) |
| SSD1306 OLED | 0x3C | 128×32 display |

Run an I2C scan if unsure — uncomment `scan: true` or add a scan sketch.

## Software Setup

### Prerequisites

1. **PlatformIO**: Install via VS Code extension or CLI (`pip install platformio`).
2. **USB driver**: The Firebeetle ESP32-E uses a **CH340** USB-serial chip.
   - **macOS**: May need the WCH driver from wch-ic.com. After install, allow
     the kernel extension in System Settings → Privacy & Security.
   - Verify: `ls /dev/cu.usb*` should show the device when plugged in.

### Build & Flash

```bash
# Clone / download this project
cd firebeetle-sensor-hub

# Create your config with real credentials
cp include/config.h.example include/config.h
# Edit include/config.h with your WiFi SSID, password, MQTT broker IP, etc.

# Build
pio run

# Flash via USB (first time)
pio run --target upload

# Monitor serial output
pio device monitor
```

After the first USB flash, subsequent updates go over WiFi via OTA:
```bash
# Uncomment the OTA lines in platformio.ini, set the device IP
pio run --target upload
```

### Configuration Checklist

Edit `include/config.h`:

| Setting | What to change | Example |
|---------|---------------|---------|
| `WIFI_SSID` | Your WiFi network name | `"MyHomeWiFi"` |
| `WIFI_PASSWORD` | Your WiFi password | `"hunter2"` |
| `MQTT_BROKER` | Home Assistant VM IP | `"192.168.1.50"` |
| `MQTT_USER` | Mosquitto username | `"mqtt_user"` |
| `MQTT_PASSWORD` | Mosquitto password | `"strongpassword"` |
| `DEVICE_NAME` | MQTT topic segment | `"office-sensor-hub"` |
| `OTA_PASSWORD` | OTA update password | `"mysecretOTA"` |

---

## Home Assistant Integration

### Step 1: Install Mosquitto MQTT Broker

If not already installed:

1. **Settings → Add-ons → Add-on Store → Mosquitto broker** → Install → Start
2. Enable **Start on Boot** and **Watchdog**
3. Create an MQTT user: **Settings → People → Users → Add User**
   - Username: `mqtt_user` (cannot use `homeassistant` or `addons`)
   - Set a strong password
   - Check "Can only log in from local network"
4. **Settings → Devices & Services → Add Integration → MQTT**
   - It should auto-discover the Mosquitto broker
   - Enable **MQTT Discovery**

### Step 2: Flash and Power the Firebeetle

After flashing, the device will:
1. Connect to WiFi (watch serial monitor for IP assignment)
2. Connect to MQTT broker
3. Publish discovery configs for all 8 entities
4. Begin publishing sensor state every 30 seconds

### Step 3: Verify in Home Assistant

Within seconds of the device connecting to MQTT, navigate to:
**Settings → Devices & Services → MQTT → Devices**

You should see **"Office Sensor Hub"** (or your configured `DEVICE_FRIENDLY_NAME`)
with these entities:

| Entity ID | Type | Unit |
|-----------|------|------|
| `sensor.office_sensor_hub_dht22_temperature` | Temperature | °C |
| `sensor.office_sensor_hub_dht22_humidity` | Humidity | % |
| `sensor.office_sensor_hub_dht11_a_temperature` | Temperature | °C |
| `sensor.office_sensor_hub_dht11_a_humidity` | Humidity | % |
| `sensor.office_sensor_hub_dht11_b_temperature` | Temperature | °C |
| `sensor.office_sensor_hub_dht11_b_humidity` | Humidity | % |
| `sensor.office_sensor_hub_illuminance` | Illuminance | lx |
| `sensor.office_sensor_hub_wifi_signal` | Signal Strength | dBm |

### Step 4: Dashboard Cards

#### Mushroom Chips — At-a-Glance Status Bar

```yaml
type: custom:mushroom-chips-card
chips:
  - type: template
    entity: sensor.office_sensor_hub_dht22_temperature
    icon: mdi:thermometer
    icon_color: |-
      {% set t = states('sensor.office_sensor_hub_dht22_temperature') | float(0) %}
      {% if t >= 28 %}red{% elif t >= 25 %}orange{% elif t < 18 %}blue{% else %}green{% endif %}
    content: "{{ states('sensor.office_sensor_hub_dht22_temperature') }}°C"
  - type: template
    entity: sensor.office_sensor_hub_dht22_humidity
    icon: mdi:water-percent
    icon_color: |-
      {% set h = states('sensor.office_sensor_hub_dht22_humidity') | float(0) %}
      {% if h > 70 %}red{% elif h > 60 %}orange{% elif h < 30 %}blue{% else %}green{% endif %}
    content: "{{ states('sensor.office_sensor_hub_dht22_humidity') }}%"
  - type: template
    entity: sensor.office_sensor_hub_illuminance
    icon: mdi:brightness-6
    icon_color: |-
      {% set l = states('sensor.office_sensor_hub_illuminance') | float(0) %}
      {% if l > 500 %}amber{% elif l > 50 %}green{% else %}blue{% endif %}
    content: "{{ states('sensor.office_sensor_hub_illuminance') }} lx"
```

#### Mini Graph Card — 24-Hour Climate History

```yaml
type: custom:mini-graph-card
name: Office Climate (24h)
entities:
  - entity: sensor.office_sensor_hub_dht22_temperature
    name: Temp (DHT22)
    color: "#ff6384"
    show_state: true
  - entity: sensor.office_sensor_hub_dht22_humidity
    name: Humidity (DHT22)
    color: "#36a2eb"
    y_axis: secondary
    show_state: true
hours_to_show: 24
points_per_hour: 2
line_width: 2
show:
  legend: true
  fill: fade
```

#### Multi-Sensor Comparison Card

Compare all three temperature sensors to cross-validate readings:

```yaml
type: custom:mini-graph-card
name: Temperature Comparison
entities:
  - entity: sensor.office_sensor_hub_dht22_temperature
    name: DHT22
    color: "#ff6384"
    show_state: true
  - entity: sensor.office_sensor_hub_dht11_a_temperature
    name: DHT11-A
    color: "#4bc0c0"
    show_state: true
  - entity: sensor.office_sensor_hub_dht11_b_temperature
    name: DHT11-B
    color: "#ff9f40"
    show_state: true
hours_to_show: 24
points_per_hour: 2
show:
  legend: true
  fill: false
```

#### Entities Card — All Sensors

```yaml
type: entities
title: Office Sensor Hub
entities:
  - entity: sensor.office_sensor_hub_dht22_temperature
    name: Temperature (Primary)
  - entity: sensor.office_sensor_hub_dht22_humidity
    name: Humidity (Primary)
  - type: divider
  - entity: sensor.office_sensor_hub_dht11_a_temperature
    name: Temp (DHT11-A)
  - entity: sensor.office_sensor_hub_dht11_a_humidity
    name: Humidity (DHT11-A)
  - entity: sensor.office_sensor_hub_dht11_b_temperature
    name: Temp (DHT11-B)
  - entity: sensor.office_sensor_hub_dht11_b_humidity
    name: Humidity (DHT11-B)
  - type: divider
  - entity: sensor.office_sensor_hub_illuminance
    name: Light Level
  - entity: sensor.office_sensor_hub_wifi_signal
    name: WiFi Signal
```

### Step 5: Automations

#### Temperature Alert

```yaml
automation:
  - alias: "Office temperature alert"
    mode: single
    triggers:
      - trigger: numeric_state
        entity_id: sensor.office_sensor_hub_dht22_temperature
        above: 28
        for: "00:05:00"
    actions:
      - action: notify.mobile_app_your_phone
        data:
          title: "Office Too Warm"
          message: >-
            Temperature: {{ states('sensor.office_sensor_hub_dht22_temperature') }}°C.
            Consider opening a window.
          data:
            tag: office_temp_alert
            priority: high
```

#### Low Light Detection

```yaml
automation:
  - alias: "Office lights needed"
    mode: restart
    triggers:
      - trigger: numeric_state
        entity_id: sensor.office_sensor_hub_illuminance
        below: 50
        for: "00:01:00"
    conditions:
      - condition: time
        after: "08:00:00"
        before: "22:00:00"
    actions:
      - action: light.turn_on
        target:
          entity_id: light.office_desk
        data:
          brightness_pct: 80
```

### Step 6: Template Sensors (optional)

Add to `configuration.yaml` for derived values:

```yaml
template:
  - sensor:
      # Average temperature across all three sensors
      - name: "Office Average Temperature"
        unique_id: office_avg_temp
        unit_of_measurement: "°C"
        device_class: temperature
        state_class: measurement
        state: >-
          {% set sensors = [
            states('sensor.office_sensor_hub_dht22_temperature') | float(none),
            states('sensor.office_sensor_hub_dht11_a_temperature') | float(none),
            states('sensor.office_sensor_hub_dht11_b_temperature') | float(none)
          ] | reject('none') | list %}
          {% if sensors | length > 0 %}
            {{ (sensors | sum / sensors | length) | round(1) }}
          {% else %}
            unavailable
          {% endif %}

      # Dew point from DHT22 (Arden Buck equation)
      - name: "Office Dew Point"
        unique_id: office_dew_point
        unit_of_measurement: "°C"
        device_class: temperature
        state_class: measurement
        state: >-
          {% set T = states('sensor.office_sensor_hub_dht22_temperature') | float %}
          {% set RH = states('sensor.office_sensor_hub_dht22_humidity') | float %}
          {% set b = 18.678 %}{% set c = 257.14 %}{% set d = 234.5 %}
          {% set gamma = log((RH/100) * e**((b-T/d)*(T/(c+T)))) %}
          {{ ((c * gamma) / (b - gamma)) | round(1) }}

      # Comfort assessment
      - name: "Office Comfort"
        unique_id: office_comfort
        state: >-
          {% set t = states('sensor.office_sensor_hub_dht22_temperature') | float %}
          {% set h = states('sensor.office_sensor_hub_dht22_humidity') | float %}
          {% if t >= 20 and t <= 24 and h >= 40 and h <= 60 %}Comfortable
          {% elif t > 28 or t < 16 or h > 70 or h < 30 %}Poor
          {% else %}Fair{% endif %}
```

### Step 7: Recorder Tuning

If not already tuned, add to `configuration.yaml` to control database growth:

```yaml
recorder:
  purge_keep_days: 10
  commit_interval: 5
  auto_purge: true
```

Sensors with `state_class: measurement` (which all of ours have via discovery)
automatically generate **long-term statistics** — hourly min/mean/max stored
indefinitely, unaffected by `purge_keep_days`. You get months of history graphs
while keeping the database lean.

---

## MQTT Topic Structure

All topics under: `home/office-sensor-hub/`

| Topic | Payload | Retained | Purpose |
|-------|---------|----------|---------|
| `.../status` | `online` / `offline` | Yes | LWT availability |
| `.../state` | JSON (all sensors) | No | Sensor telemetry |

State payload example:
```json
{
  "dht22_temperature": 22.3,
  "dht22_humidity": 48.1,
  "dht11a_temperature": 22.0,
  "dht11a_humidity": 50.0,
  "dht11b_temperature": 21.8,
  "dht11b_humidity": 49.0,
  "illuminance": 342.5,
  "wifi_rssi": -42
}
```

HA discovery topics (published retained on connect):
```
homeassistant/sensor/firebeetle_office_01/dht22_temperature/config
homeassistant/sensor/firebeetle_office_01/dht22_humidity/config
homeassistant/sensor/firebeetle_office_01/dht11a_temperature/config
homeassistant/sensor/firebeetle_office_01/dht11a_humidity/config
homeassistant/sensor/firebeetle_office_01/dht11b_temperature/config
homeassistant/sensor/firebeetle_office_01/dht11b_humidity/config
homeassistant/sensor/firebeetle_office_01/illuminance/config
homeassistant/sensor/firebeetle_office_01/wifi_rssi/config
```

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| No serial output | Wrong baud rate | Set monitor to 115200 |
| `[WiFi] Connecting...` loops | Wrong SSID/password | Check `config.h` |
| `[MQTT] Failed, rc=-2` | Broker unreachable | Verify HA IP, port 1883, Mosquitto running |
| `[MQTT] Failed, rc=5` | Auth rejected | Check MQTT username/password, ensure user exists in HA |
| `[Sensor] dht22_temperature: invalid` | Wiring issue | Check pull-up resistor, data pin, power |
| `[OLED] SSD1306 init FAILED` | I2C address mismatch | Run I2C scan; try 0x3C or 0x3D |
| Device not appearing in HA | Discovery disabled | Settings → Integrations → MQTT → Enable discovery |
| Stale/ghost entities in HA | Old retained messages | Publish empty payload to the config topic |

### I2C Scanner

If I2C devices aren't detected, flash this temporary sketch to find addresses:

```cpp
#include <Wire.h>
void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
            Serial.printf("Found device at 0x%02X\n", addr);
    }
}
void loop() {}
```

## Project Structure

```
firebeetle-sensor-hub/
├── platformio.ini              # Build config + library deps
├── include/
│   ├── config.h.example        # Template (committed)
│   ├── config.h                # Real credentials (.gitignore'd)
│   ├── pins.h                  # Firebeetle GPIO assignments
│   └── version.h               # Firmware semver
├── src/
│   ├── main.cpp                # Entry point — init + loop dispatch
│   ├── timer_utils.h           # Non-blocking millis() timer
│   ├── wifi_manager.h/.cpp     # WiFi with event-driven reconnection
│   ├── mqtt_client.h/.cpp      # MQTT + HA auto-discovery
│   ├── ota_handler.h/.cpp      # ArduinoOTA wrapper
│   ├── sensor_hub.h/.cpp       # Sensor registry + scheduler
│   ├── oled_display.h/.cpp     # SSD1306 128×32 page-cycling display
│   └── sensors/
│       ├── base_sensor.h       # Abstract sensor interface
│       ├── dht_sensor.h        # DHT22/DHT11 driver (shared device)
│       └── bh1750_sensor.h     # BH1750 I2C lux driver
├── .gitignore
└── README.md
```

## Extending

To add a new sensor:

1. Create `src/sensors/my_sensor.h` inheriting from `BaseSensor`
2. Implement `name()`, `begin()`, `read()`, `range_min()`, `range_max()`
3. Register in `main.cpp`: `sensor_hub::add_sensor(std::make_unique<MySensor>(args))`
4. Add its discovery call in `mqtt_client.cpp` → `publish_discovery()`
5. Add its JSON field in `main.cpp` → `publish_sensor_state()`
6. Add library to `platformio.ini` → `lib_deps`

## Licence

MIT
