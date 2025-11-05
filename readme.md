
# ESPHome MR24FDB1 — Seeed 24 GHz Fall & Presence Radar (UART) External Component

![MR24FDB1 on ESP32](./mr24fdb1.jpg)

An **ESPHome external component** for the Seeed Studio **MR24FDB1** 24 GHz radar over **UART**. It exposes:

* `binary_sensor.presence` – occupancy/presence (for automations like lights)
* `binary_sensor.fall_detected` – fall event (real/suspected → binary)
* `text_sensor.environment_triplet` – raw triplet (`00 FF FF`, `01 00 FF`, `01 01 01`, …)
* `text_sensor.environment_label` – human-readable label (Unoccupied / Someone stationary / Someone moving / Occupied (other))
* `text_sensor.approach_state` – None / Close / Away
* `text_sensor.fall_state` – None / Suspected / Real
* `sensor.sign_parameter` – activity / “body sign” (0–100 or float)

> Uses ESPHome **external_components** (the modern, supported mechanism) rather than deprecated “custom components”. ([esphome.io][1])

---

## Features & State Mapping

**Environment status** (`func=0x04/0x03`, `a1=0x03`, `a2=0x05`, `DATA[0..2]`):

* `00 FF FF` → **Unoccupied**
* `01 00 FF` → **Someone stationary**
* `01 01 01` → **Someone moving**
* (others) → **Occupied (other)**

**Approach/Away** (`func=0x04/0x03`, `a1=0x03`, `a2=0x07`, `DATA[0]`):
`0x01` = None, `0x02` = Close, `0x03` = Away.

**Sign parameter** (`func=0x04/0x03`, `a1=0x06`): typically a 4-byte float (LE) or a single byte 0..100. Convention here: `0`=no person, `1`=stationary, `2..100`=activity.

**Fall reporting** (`func=0x06`, `a1=0x01`, `DATA[0]`):
`0x00` = Suspected, `0x01` = Real, `0x02` = None.

> The component also uses ESPHome’s UART debugger helpers (e.g. `UARTDebug::log_hex(direction, bytes, separator)`) for diagnostics. ([esphome.io][2])

---

## Hardware Requirements

* **ESP32** (tested on **NodeMCU-32S**)
* **UART** connection to MR24FDB1 @ `9600 8N1`
* **ESPHome 2024.8+** (also tested on 2025.10.x) ([esphome.io][3])

**Default pins (edit as needed):** `RX=GPIO16`, `TX=GPIO17`.

---

## Installation

### A) Local (recommended for development)

Copy the `components/` folder into your ESPHome project directory (next to your `my_node.yaml`) so you end up with:

```
<project>/
├─ components/
│  └─ mr24fdb1/
│     ├─ component.yaml
│     ├─ mr24fdb1.h
│     └─ mr24fdb1.cpp
└─ my_node.yaml
```

In your `my_node.yaml` add:

```yaml
external_components:
  - source:
      type: local
      path: components   # this folder contains mr24fdb1/

uart:
  id: radar_uart
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 9600
  parity: NONE
  stop_bits: 1

mr24fdb1:
  uart_id: radar_uart

binary_sensor:
  - platform: mr24fdb1
    presence:
      name: "${device_name} Presence"
    fall_detected:
      name: "${device_name} Fall Detected"

text_sensor:
  - platform: mr24fdb1
    environment_triplet:
      name: "${device_name} Environment Triplet"
    environment_label:
      name: "${device_name} Environment Label"
    approach_state:
      name: "${device_name} Approach State"
    fall_state:
      name: "${device_name} Fall State"

sensor:
  - platform: mr24fdb1
    sign_parameter:
      name: "${device_name} Sign Parameter"

logger:
  level: DEBUG
  # If you enable UART debugger in YAML elsewhere, it uses helpers like UARTDebug::log_hex(...)
```

> Learn more about **external_components** in ESPHome docs. ([esphome.io][1])

---

## Example

See [`examples/esp32_mr24fdb1.yaml`](./examples/esp32_mr24fdb1.yaml) for a complete working node config (pins, Wi-Fi, OTA, API, etc.).

---

## Troubleshooting

* **CRC mismatch in logs**: briefly appears when partial frames are read; should stabilize after a few frames. Persistent CRC errors usually mean wrong baud/pins or wiring noise.
* **No state updates**: verify UART pins, `baud_rate: 9600`, common GND, and that the module is powered per datasheet.
* **Long UART debug lines get truncated**: increase logger buffer size or lower log level. ([esphome.io][2])
* **ESPHome didn’t pick up source changes**: run ***Clean build files*** in ESPHome UI and flash again.

---

## Roadmap

* Optional config for per-state debounce
* Expose more raw registers as sensors
* Unit tests for frame parser

---

## License

MIT (see [`LICENSE`](./LICENSE)).

---

## Acknowledgments

* ESPHome docs & dev guides for external components and UART helpers. ([esphome.io][1])
