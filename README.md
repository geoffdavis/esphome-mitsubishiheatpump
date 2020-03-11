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
 * ESP8266 only: `logger: baud\_rate: 0` - disables serial port logging on sole hardware UART

```yaml
esphome:
  libraries:
  - https://github.com/geoffdavis/HeatPump#init_fix
  - https://github.com/geoffdavis/esphome-mitsubishiheatpump

# Enable logging
logger:
	# ESP8266 ONLY:
	# disable serial port logging, as the HeatPump component needs
	# the sole hardware UART on the ESP8266
	baud_rate: 0

climate:
  - platform: custom
    lambda: |-
      auto my_heatpump = new MitsubishiHeatPump(&Serial);
      App.register_component(my_heatpump);
      return {my_heatpump};

    climates:
      - name: "My Heat Pump"
```

Note that you can change the &
