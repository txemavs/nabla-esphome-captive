# nabla-esphome-captive

ESPHome external component: Nabla-branded Wi‑Fi captive portal for nabla-mandos and other Nabla devices.

## Overview

This component provides a custom captive portal with Nabla branding (hollow blue nabla ∇ triangle logo) that replaces the stock ESPHome captive portal appearance while maintaining full compatibility with the WiFi save functionality.

**Key features:**
- Nabla-branded UI with dark theme and blue (#1a5cff) accents
- Minimal footprint: **~2.2 KB gzipped** (well under 40 KB target)
- Same WiFi scan/save behavior as stock ESPHome captive portal
- Works on ESP32, ESP8266, BK72XX, LN882X, RP2040, RTL87XX
- No external dependencies (no Vuetify/Ionic)
- Self-contained HTML/CSS/JS

## Installation

Add the external component to your device YAML:

```yaml
external_components:
  - source: github://txemavs/nabla-esphome-captive@main
    components: [nabla_captive]
```

## Usage

### Basic Configuration

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "MyDevice-Fallback"
    password: "fallback123"

# Keep stock captive_portal for DNS/AP wiring (recommended)
captive_portal:

# Add Nabla branded portal
nabla_captive:
```

### Minimal Configuration (AP-only fallback)

```yaml
wifi:
  ap:
    ssid: "Nabla-Setup"
    password: "nablasetup"

captive_portal:
nabla_captive:
```

## How It Works

1. When your device cannot connect to the configured WiFi network for ~1 minute, it starts a fallback WiFi access point
2. When users connect to this AP, the captive portal automatically opens (or navigate to http://192.168.4.1/)
3. The Nabla-branded portal displays available networks and allows entering credentials
4. Credentials are saved and the device attempts to connect

## Architecture Choice

This component uses **Approach A: Separate Component** rather than overriding `captive_portal`:

- `nabla_captive` is a standalone component that depends on `wifi` and `web_server_base`
- You keep the stock `captive_portal:` entry for DNS server and AP detection wiring
- The `nabla_captive` component serves the branded HTML page

This approach was chosen because:
1. More reliable with ESPHome updates (no component override conflicts)
2. Cleaner separation of concerns
3. Allows future customization without breaking core functionality

## Important Notes

### WiFi Credentials Persistence

**Settings saved via the portal will be overwritten by the next flash unless you update your YAML.**

The captive portal saves credentials to flash memory (NVS on ESP32). However, when you compile and upload new firmware, ESPHome writes the credentials from your YAML file, overwriting any portal-saved settings.

To make portal-saved credentials permanent:
1. Note the SSID/password you entered in the portal
2. Update your YAML with those credentials
3. Re-flash the device

### Compatibility

- **ESPHome version:** 2024.x and 2025.x
- **Platforms:** ESP32, ESP8266, BK72XX, LN882X, RP2040, RTL87XX
- **Frameworks:** Arduino and ESP-IDF (ESP32)

## File Structure

```
components/
└── nabla_captive/
    ├── __init__.py           # ESPHome component config/codegen
    ├── nabla_captive.h       # C++ header
    ├── nabla_captive.cpp     # C++ implementation
    ├── nabla_index.h         # Gzipped HTML as PROGMEM array
    ├── dns_server_esp32_idf.h   # DNS server for ESP-IDF
    ├── dns_server_esp32_idf.cpp # DNS server implementation
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
   print('// Generated from index.html')
   print('#include \"esphome/core/hal.h\"')
   print('namespace esphome::nabla_captive {')
   print('constexpr uint8_t NABLA_INDEX_GZ[] PROGMEM = {')
   for i, b in enumerate(data):
       if i % 16 == 0: print('    ', end='')
       print(f'0x{b:02x}', end='')
       if i < len(data) - 1: print(', ', end='')
       if (i + 1) % 16 == 0 or i == len(data) - 1: print()
   print('};')
   print(f'// Size: {len(data)} bytes')
   print('}  // namespace esphome::nabla_captive')
   " > nabla_index.h
   rm index.html.gz
   ```

## API Endpoints

The component uses the same endpoints as stock ESPHome captive portal:

- `GET /config.json` - Returns device name, MAC, and scanned networks
- `GET /wifisave?ssid=...&psk=...` - Saves WiFi credentials

## License

MIT License - Same as ESPHome
