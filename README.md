# esphome-mitsubishiheatpump

Wirelessly control your Mitsubishi Comfort HVAC equipment with an ESP8266 or
ESP32 using the [ESPHome](https://esphome.io) framework.

## Features
* Instant feedback of command changes via RF Remote to HomeAssistant or MQTT.
* Direct control without the remote.
* Uses the [SwiCago/HeatPump](https://github.com/SwiCago/HeatPump) Arduino
  libary to talk to the unit directly via the internal `CN105` connector.

## Requirements
* https://github.com/SwiCago/HeatPump
* ESPHome 1.18.0 or greater

## Supported Microcontrollers
This library should work on most ESP8266 or ESP32 platforms. It has been tested
with the following MCUs:
* Generic ESP-01S board (ESP8266)
* WeMos D1 Mini (ESP8266)
* Generic ESP32 Dev Kit (ESP32)

## Supported Mitsubishi Climate Units
The underlying HeatPump library works with a number of Mitsubishi HVAC
units. Basically, if the unit has a `CN105` header on the main board, it should
work with this library. The [HeatPump
wiki](https://github.com/SwiCago/HeatPump/wiki/Supported-models) has a more
exhaustive list.

The same `CN105` connector is used by the Mitsubishi KumoCloud remotes, which
have a
[compatibility list](https://www.mitsubishicomfort.com/kumocloud/compatibility)
available.

The whole integration with this libary and the underlying HeatPump has been
tested by the author on the following units:
* `MSZ-GL06NA`
* `MFZ-KA09NA`

## Usage
### Step 1: Build a control circuit.

Build a control circuit with your MCU as detailed in the [SwiCago/HeatPump
 README](https://github.com/SwiCago/HeatPump/blob/master/README.md#demo-circuit).
You can use either an ESP8266 or an ESP32 for this.

Note: several users have reported that they've been able to get away with
not using the pull-up resistors, and just [directly connecting a Wemos D1 mini
to the control
board](https://github.com/SwiCago/HeatPump/issues/13#issuecomment-457897457)
via CN105.

### Step 2: Use ESPHome 1.18.0 or higher

The code in this repository makes use of a number of features in the 1.18.0
version of ESPHome, including various Fan modes and external components.

### Step 3: Add this repository as an external component

Add this repository to your ESPHome config:

```yaml
external_components:
  - source: github://geoffdavis/esphome-mitsubishiheatpump
```

### Step 4: Configure the heatpump

Add a `mitsubishi_heatpump` to your ESPHome config:

```yaml
climate:
  - platform: mitsubishi_heatpump
    name: "My Heat Pump"

    # Optional
    hardware_uart: UART0

    # Optional
    update_period: 500ms
```

On ESP8266 you'll need to disable logging to serial because it conflicts with
the heatpump UART:

```yaml
logger:
  baud_rate: 0
```

On ESP32 you can change `hardware_uart` to `UART1` or `UART2` and keep logging
enabled on the main serial port.

Note: this component DOES NOT use the ESPHome `uart` component, as it requires
direct access to a hardware UART via the Arduino `HardwareSerial` class. The
Mitsubishi Heatpump units use an atypical serial port setting ("even parity").
Parity bit support is not implemented in any of the existing software serial
libraries, including the one in ESPHome. There's currently no way to guarantee
access to a hardware UART nor retrieve the `HardwareSerial` handle from the
`uart` component within the ESPHome framework.

# Example configuration

Below is an example configuration which will include wireless strength
indicators and permit over the air updates. You'll need to create a
`secrets.yaml` file inside of your `esphome` directory with entries for the
various items prefixed with `!secret`.

```yaml
esphome:
  name: denheatpump
  platform: ESP8266
  board: esp01_1m
  # Boards tested: ESP-01S (ESP8266), Wemos D1 Mini (ESP8266); ESP32 Wifi-DevKit2

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

external_components:
  - source: github://geoffdavis/esphome-mitsubishiheatpump

climate:
  - platform: mitsubishi_heatpump
    name: "Den Heat Pump"

    # ESP32 only - change UART0 to UART1 or UART2 and remove the
    # logging:baud_rate above to allow the built-in UART0 to function for
    # logging.
    hardware_uart: UART0
```

# Advanced

Some models of heat pump require different baud rates or don't support all
possible modes of operation. You can configure pretty much everything in YAML
to match what your hardware supports. For example:

```yaml
climate:
  - platform: mitsubishi_heatpump
    name: "My heat pump"
    hardware_uart: UART2
    baud_rate: 9600
    supports:
      mode: [AUTO, COOL, HEAT, FAN_ONLY]
      fan_mode: [AUTO, LOW, MEDIUM, HIGH]
      swing_mode: [OFF, VERTICAL]
    visual:
      min_temperature: 16
      max_temperature: 31
      temperature_step: 1.0
```

# See Also

## Other Implementations
The [gysmo38/mitsubishi2MQTT](https://github.com/gysmo38/mitsubishi2MQTT)
  Arduino sketch also uses the `SwiCago/HeatPump`
library, and works with MQTT directly. The author found it's WiFi stack to not
be particularly robust, but the controls worked fine. Like this ESPHome
repository, it will automatically register the device in your HomeAssistant
instance if you have HA configured to do so.

There's also the built-in to ESPHome
[Mitsubishi](https://github.com/esphome/esphome/blob/dev/esphome/components/mitsubishi/mitsubishi.h)
climate component.
The big drawback with the built-in component is that it uses Infrared Remote
commands to talk to the Heat Pump. By contrast, the approach used by this
repository and it's underlying `HeatPump` library allows bi-directional
communication with the Mitsubishi system, and can detect when someone changes
the settings via an IR remote.

## Reference documentation

The author referred to the following documentation repeatedly:
* https://esphome.io/components/sensor/custom.html
* https://esphome.io/components/climate/custom.html
* Source for ESPHome's Dev branch: https://github.com/esphome/esphome/tree/dev/esphome/components/climate
