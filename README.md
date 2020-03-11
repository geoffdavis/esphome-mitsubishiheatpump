# esphome-mitsubishiheatpump
ESPHome Climate Component for Mitsubishi Heatpumps using direct serial connection

Wirelessly control your Mitsubishi Comfort HeatPump with an ESP8266 or ESP32 using the [ESPHome](https://esphome.io) framework.

## Features
* Instant feedback of command changes via RF Remote to HomeAssistant or MQTT.
* Direct control without the remote.
* Uses the [SwiCago/HeatPump](https://github.com/SwiCago/HeatPump) Arduino libary to talk to the unit directly via the internal CN105 header.
  NOTE: REQUIRES SEVERAL FIXES - SEE [PR #155](https://github.com/SwiCago/HeatPump/pull/155)

## Supported Microcontrollers
This library should work on most ESP8266 or ESP32 platforms. It has been tested
with the following MCUs:
* Generic ESP-01S board (ESP8266)
* WeMos D1 Mini (ESP8266)
* Generic ESP32 Dev Kit (ESP32)

## Supported Mitsubishi Climate Units
The underlying HeatPump library works with a number of Mitsubishi HeatPump
units. Basically, if the unit has a CN105 header on the main board, it should
work with this library. The [HeatPump
wiki](https://github.com/SwiCago/HeatPump/wiki/Supported-models) has a more
exhaustive list.

The whole integration with this libary and the underlying HeatPump has been
tested by the author on the following units:
* `MSZ-GL06NA`
* `MFZ-KA09NA`

## Usage
### Step 1: Build a control circuit as detailed in the SwiCago/HeatPump README.
You can use either an ESP8266 or an ESP32 for this.

### Step 2: Clone this repository into your ESPHome configuration directory

This repository needs to live in your ESPHome configuration directory, as it
doesn't work correctly when used as a Platform.IO library, and there doesn't
seem to be an analog for that functionality for ESPHome code.

On Hass.IO, you'll want to do something like:

* Change directories to your esphome configuration directory.
* `mkdir -p src`
* `cd src`
* `git clone https://github.com/geoffdavis/esphome-mitsubishiheatpump.git`

### Step 3: Configure your ESPHome device with YAML

Note: this component DOES NOT use the ESPHome `uart` component, as it requires
direct access to a hardware UART via the Arduino `HardwareSerial` class. The
Mitsubishi Heatpump units use an atypical serial port setting ("even parity").
Parity bit support is not implemented in any of the existing software serial
libraries, including the one in ESPHome.

There's currently no way to guarantee access to a hardware UART nor retrieve
the `HardwareSerial` handle within the ESPHome framework.

Create an ESPHome YAML configuration with the following sections:
 * `esphome: libraries:`
 * `climate:` - set up a custom climate, change the Serial port as needed.
 * ESP8266 only: `logger: baud_rate: 0` - disables serial port logging on the
   sole ESP8266 hardware UART

```yaml
esphome:
  name: denheatpump
  platform: ESP8266
  board: esp01_1m
  # Boards tested: ESP-01S (ESP8266), Wemos D1 Mini (ESP8266); ESP32 Wifi-DevKit2

  libraries:
    #- SwiCago/HeatPump
    - https://github.com/geoffdavis/HeatPump#init_fix

  includes:
    - src/esphome-mitsubishiheatpump

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: "Denheatpump Fallback Hotspot"
    password: !secret fallback_password

captive_portal:

# Enable logging
logger:
  # ESP8266 only - disable serial port logging, as the HeatPump component
  # needs the sole hardware UART on the ESP8266
  baud_rate: 0

# Enable Home Assistant API
api:

ota:

# Enable Web server.
web_server:
  port: 80

  # Sync time with Home Assistant.
time:
  - platform: homeassistant
    id: homeassistant_time

# Text sensors with general information.
text_sensor:
  # Expose ESPHome version as sensor.
  - platform: version
    name: denheatpump ESPHome Version
  # Expose WiFi information as sensors.
  - platform: wifi_info
    ip_address:
      name: denheatpump IP
    ssid:
      name: denheatpump SSID
    bssid:
      name: denheatpump BSSID

# Sensors with general information.
sensor:
  # Uptime sensor.
  - platform: uptime
    name: denheatpump Uptime

  # WiFi Signal sensor.
  - platform: wifi_signal
    name: denheatpump WiFi Signal
    update_interval: 60s


climate:
  - platform: custom
    # ESP32 only - change &Serial to &Serial1 or &Serial2 and remove the
    # logging:baud_rate above to allow the built-in UART0 to function for
    # logging.
    lambda: |-
      auto my_heatpump = new MitsubishiHeatPump(&Serial);
      App.register_component(my_heatpump);
      return {my_heatpump};
    climates:
      - name: "Den Heat Pump"
```

# See Also

The [gysmo38/mitsubishi2MQTT](https://github.com/gysmo38/mitsubishi2MQTT)
  Arduino sketch also uses the `SwiCago/HeatPump`
library, and works with MQTT directly. I found it's WiFi stack to not be
particularly robust, but the controls worked fine. Like this ESPHome
repository, it will automatically register the device in your HomeAssistant
instance if you have HA configured to do so.

There's also the built-in to ESPHome
[Mitsubishi](https://github.com/esphome/esphome/blob/dev/esphome/components/mitsubishi/mitsubishi.h)
climate component. It's only in the `dev` branch at the moment (2020-03-11).
The big drawback with the built-in component is that it uses Infrared Remote
commands to talk to the Heat Pump. By contrast, the approach used by this
repository and it's underlying `HeatPump` library allows bi-directional
communication with the Mitsubishi system, and can detect when someone changes
the settings via an IR remote.
