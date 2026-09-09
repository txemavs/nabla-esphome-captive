# nabla-esphome-captive

ESPHome external component: Nabla-branded Wi‑Fi captive portal for nabla-mandos and other Nabla devices.

## Overview

This component **replaces** the stock ESPHome `captive_portal` with a Nabla-branded dark UI. It provides the same WiFi scan/save functionality with custom styling.

**Key features:**
- Nabla-branded UI with Agency dark theme (black background, blue #1a5cff accents)
- Small nabla ∇ logo at top (hollow blue triangle pointing down)
- Minimal footprint: **~2.2 KB gzipped**
- Same WiFi scan/save behavior as stock ESPHome captive portal
- Works on ESP32, ESP8266, BK72XX, RTL87XX
- No external dependencies (no Vuetify/Ionic)

## Installation

Add the external component to your device YAML:

```yaml
external_components:
  - source: github://txemavs/nabla-esphome-captive@main
    components: [nabla_captive]
```

## Usage

**Important:** Use `nabla_captive:` instead of (not alongside) the stock `captive_portal:`. They both run DNS servers and HTTP handlers — using both wastes resources and can conflict.

### Basic Configuration

```yaml
external_components:
  - source: github://txemavs/nabla-esphome-captive@main
    components: [nabla_captive]

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "MyDevice-Fallback"
    password: "fallback123"

# Use nabla_captive INSTEAD OF captive_portal
nabla_captive:
```

### Minimal Configuration (AP-only setup mode)

```yaml
external_components:
  - source: github://txemavs/nabla-esphome-captive@main
    components: [nabla_captive]

wifi:
  ap:
    ssid: "Nabla-Setup"
    password: "nablasetup"

nabla_captive:
```

## How It Works

1. When your device cannot connect to the configured WiFi network for ~1 minute, it starts a fallback WiFi access point
2. When users connect to this AP, the captive portal automatically opens (or navigate to http://192.168.4.1/)
3. The Nabla-branded portal displays available networks and allows entering credentials
4. Credentials are saved and the device attempts to connect

## Important Notes

### Do NOT use both `captive_portal:` and `nabla_captive:`

This component fully replaces the stock captive portal. Using both will:
- Run two DNS servers on port 53 (conflict)
- Double the socket usage
- Cause undefined behavior

**Correct:**
```yaml
nabla_captive:
```

**Wrong:**
```yaml
captive_portal:    # Don't include this
nabla_captive:
```

### WiFi Credentials Persistence

**Settings saved via the portal will be overwritten by the next flash unless you update your YAML.**

The captive portal saves credentials to flash memory (NVS on ESP32). However, when you compile and upload new firmware, ESPHome writes the credentials from your YAML file, overwriting any portal-saved settings.

To make portal-saved credentials permanent:
1. Note the SSID/password you entered in the portal
2. Update your YAML with those credentials
3. Re-flash the device

### Compatibility

- **ESPHome version:** 2024.6.x and later
- **Platforms:** ESP32, ESP8266, BK72XX, RTL87XX
- **Framework:** Arduino

## File Structure

```
components/
└── nabla_captive/
    ├── __init__.py           # ESPHome component config/codegen
    ├── nabla_captive.h       # C++ header
    ├── nabla_captive.cpp     # C++ implementation
    ├── nabla_index.h         # Gzipped HTML as PROGMEM array
    └── index.html            # Source HTML (for reference)
```

## Customization

To modify the portal appearance:

1. Edit `components/nabla_captive/index.html`
2. Regenerate the gzipped header:
   ```bash
   cd components/nabla_captive
   gzip -9 -c index.html > index.html.gz
   python3 -c "
   with open('index.html.gz', 'rb') as f:
       data = f.read()
   print('#pragma once')
   print('#include \"esphome/core/hal.h\"')
   print('namespace esphome {')
   print('namespace nabla_captive {')
   print('const uint8_t NABLA_INDEX_GZ[] PROGMEM = {')
   for i, b in enumerate(data):
       if i % 16 == 0: print('    ', end='')
       print(f'0x{b:02x}', end='')
       if i < len(data) - 1: print(', ', end='')
       if (i + 1) % 16 == 0 or i == len(data) - 1: print()
   print('};')
   print('}  // namespace nabla_captive')
   print('}  // namespace esphome')
   " > nabla_index.h
   rm index.html.gz
   ```

## API Endpoints

The component uses the same endpoints as stock ESPHome captive portal:

- `GET /` - Serves the Nabla-branded HTML page
- `GET /config.json` - Returns device name, MAC, and scanned networks
- `GET /wifisave?ssid=...&psk=...` - Saves WiFi credentials

## License

MIT License - Same as ESPHome
