/**
 * espmhp.h
 *
 * Header file for esphome-mitsubishiheatpump
 *
 * Author: Geoff Davis.<geoff@geoffdavis.com>
 * Date: 2020-03-11
 * License: BSD
 *
 * Requirements:
 * - https://github.com/SwiCago/HeatPump
 * - ESPHome 1.15.0-dev or greater
 */

#define USE_CALLBACKS

#include "esphome.h"
#include "HeatPump.h"
using namespace esphome;

#ifndef ESPMHP_H
#define ESPMHP_H

static const char* TAG = "MitsubishiHeatPump"; // Logging tag

static const char* ESPMHP_VERSION = "1.1.0";

/* If polling interval is greater than 9 seconds, the HeatPump
library reconnects, but doesn't then follow up with our data request.*/
static const uint32_t ESPMHP_POLL_INTERVAL_DEFAULT = 500; // in milliseconds,
                                                           // 0 < X <= 9000
static const uint8_t ESPMHP_MIN_TEMPERATURE = 16; // degrees C,
                                                  // defined by hardware
static const uint8_t ESPMHP_MAX_TEMPERATURE = 31; // degrees C,
                                                  //defined by hardware
static const uint8_t ESPMHP_TEMPERATURE_STEP = 0.5; // temperature setting step,
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

        // Debugging function to print the object's state.
        void dump_state();

        // Handle a request from the user to change settings.
        void control(const climate::ClimateCall &call) override;

    protected:
        // HeatPump object using the underlying Arduino library.
        HeatPump* hp;

        // Allow the HeatPump class to use get_hw_serial_
        friend class HeatPump;

        //Accessor method for the HardwareSerial pointer
        HardwareSerial* get_hw_serial_() {
            return this->hw_serial_;
        }

        //Print a warning message if we're using the sole hardware UART on an
        //ESP8266 or UART0 on ESP32
        void check_logger_conflict_();

    private:
        // Retrieve the HardwareSerial pointer from friend and subclasses.
        HardwareSerial *hw_serial_;

};

#endif
