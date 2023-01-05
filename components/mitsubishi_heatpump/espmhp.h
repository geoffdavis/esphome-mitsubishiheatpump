/**
 * espmhp.h
 *
 * Header file for esphome-mitsubishiheatpump
 *
 * Author: Geoff Davis <geoff@geoffdavis.com>
 * Author: Phil Genera @pgenera on Github.
 * Author: @nao-pon on Github
 * Author: Simon Knopp @sijk on Github
 * Last Updated: 2021-06-23
 * License: BSD
 *
 * Requirements:
 * - https://github.com/SwiCago/HeatPump
 * - ESPHome 1.19.1 or greater
 */

#define USE_CALLBACKS

#include "esphome.h"
#include "esphome/core/preferences.h"

#include "HeatPump.h"
using namespace esphome;

#ifndef ESPMHP_H
#define ESPMHP_H

static const char* TAG = "MitsubishiHeatPump"; // Logging tag

static const char* ESPMHP_VERSION = "2.4.1";

/* If polling interval is greater than 9 seconds, the HeatPump
library reconnects, but doesn't then follow up with our data request.*/
static const uint32_t ESPMHP_POLL_INTERVAL_DEFAULT = 500; // in milliseconds,
                                                           // 0 < X <= 9000
static const uint8_t ESPMHP_MIN_TEMPERATURE = 16; // degrees C,
                                                  // defined by hardware
static const uint8_t ESPMHP_MAX_TEMPERATURE = 31; // degrees C,
                                                  //defined by hardware
static const float   ESPMHP_TEMPERATURE_STEP = 0.5; // temperature setting step,
                                                    // in degrees C

class MitsubishiHeatPump : public PollingComponent, public climate::Climate {

    public:

        /**
         * Create a new MitsubishiHeatPump object
         *
         * Args:
         *   hw_serial: pointer to an Arduino HardwareSerial instance
         *   poll_interval: polling interval in milliseconds
         */
        MitsubishiHeatPump(
            HardwareSerial* hw_serial,
            uint32_t poll_interval=ESPMHP_POLL_INTERVAL_DEFAULT
        );

        // Print a banner with library information.
        void banner() {
            ESP_LOGI(TAG, "ESPHome MitsubishiHeatPump version %s",
                    ESPMHP_VERSION);
        }

        // Set the baud rate. Must be called before setup() to have any effect.
        void set_baud_rate(int);

        // Set the temperature offset
        void set_temperature_offset(float);

        // print the current configuration
        void dump_config() override;

        // handle a change in settings as detected by the HeatPump library.
        void hpSettingsChanged();

        // Handle a change in status as detected by the HeatPump library.
        void hpStatusChanged(heatpumpStatus currentStatus);

        // Set up the component, initializing the HeatPump object.
        void setup() override;

        // This is called every poll_interval.
        void update() override;

        // Configure the climate object with traits that we support.
        climate::ClimateTraits traits() override;

        // Get a mutable reference to the traits that we support.
        climate::ClimateTraits& config_traits();

        // Debugging function to print the object's state.
        void dump_state();

        // Handle a request from the user to change settings.
        void control(const climate::ClimateCall &call) override;

        // Use the temperature from an external sensor. Use
        // set_remote_temp(0) to switch back to the internal sensor.
        void set_remote_temperature(float);

    protected:
        // HeatPump object using the underlying Arduino library.
        HeatPump* hp;

        // The ClimateTraits supported by this HeatPump.
        climate::ClimateTraits traits_;

        // Allow the HeatPump class to use get_hw_serial_
        friend class HeatPump;

        //Accessor method for the HardwareSerial pointer
        HardwareSerial* get_hw_serial_() {
            return this->hw_serial_;
        }

        //Print a warning message if we're using the sole hardware UART on an
        //ESP8266 or UART0 on ESP32
        void check_logger_conflict_();

        // various prefs to save mode-specific temperatures, akin to how the IR
        // remote works.
        ESPPreferenceObject cool_storage;
        ESPPreferenceObject heat_storage;
        ESPPreferenceObject auto_storage;

        optional<float> cool_setpoint;
        optional<float> heat_setpoint;
        optional<float> auto_setpoint;

        static void save(float value, ESPPreferenceObject& storage);
        static optional<float> load(ESPPreferenceObject& storage);

    private:
        // Retrieve the HardwareSerial pointer from friend and subclasses.
        HardwareSerial *hw_serial_;
        int baud_ = 0;
        optional<float> temperature_offset_;
};

#endif
