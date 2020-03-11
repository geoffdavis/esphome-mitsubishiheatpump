# esphome-mitsubishiheatpump
ESPHome Climate Component for Mitsubishi Heatpumps using direct serial connection

Wirelessly control your Mitsubishi Comfort HeatPump with an ESP8266 or ESP32 using the [ESPHome](https://esphome.io) framework.

## Features
* Instant feedback of command changes via RF Remote to HomeAssistant or MQTT.
* Direct control without the remote.
* Uses the [SwiCago/HeatPump](https://github.com/SwiCago/HeatPump) Arduino libary to talk to the unit directly via the internal J105 header.
* NOTE: REQUIRES SEVERAL FIXES - SEE https://github.com/SwiCago/HeatPump/pull/155

## Usage
### Build a control circuit as detailed in the SwiCago/HeatPump README.
You can use either an ESP8266 or an ESP32.

### Clone this repository into your ESPHome configuration directory

This repository needs to live in your ESPHome configuration directory, as it doesn't work correctly when used as a Platform.IO library, and there doesn't seem to be an analog for that functionality for esphome code.

On Hass.IO, you'll want to do something like:

* Change directories to your esphome configuration directory.
* `mkdir -p src`
* `cd src`
* `git clone https://github.com/geoffdavis/esphome-mitsubishiheatpump.git`

### Configure your ESPHome device with YAML

Note: this component DOES NOT use the ESPHome `uart` component, as it requires
direct access to a hardware UART via the Arduino `HardwareSerial` class. The
Mitsubishi Heatpump uses an untypical serial port setting, which are not
implemented in any of the existing software serial libraries.

There's currently no way to guarantee access to a hardware UART nor retrieve
the `HardwareSerial` handle within the ESPHome framework.

Create an ESPHome YAML configuration with the following sections:
 * `esphome: libraries:`
 * `climate:` - set up a custom climate, change the Serial port as needed.
 * ESP8266 only: `logger: baud\_rate: 0` - disables serial port logging on the
   sole ESP8266 hardware UART

```yaml
esphome:
  libraries:
  #- SwiCago/HeatPump
  - https://github.com/geoffdavis/HeatPump#init_fix
  includes:
  - src/esphome-mitsubishiheatpump

# Enable logging
logger:
  # ESP8266 only - disable serial port logging, as the HeatPump component
  # needs the sole hardware UART on the ESP8266
  baud_rate: 0

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
      - name: "Den Mini-Split Heat Pump"
```
