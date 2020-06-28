/**
 * espmhp.cpp
 *
 * Implementation of esphome-mitsubishiheatpump
 *
 * Author: Geoff Davis.<geoff@geoffdavis.com>
 * Author: Phil Genera @pgenera on Github.
 * Last Updated: 2020-06-02
 * License: BSD
 *
 * Requirements:
 * - https://github.com/SwiCago/HeatPump
 * - ESPHome 1.15.0-dev or greater
 */

#include "espmhp.h"
using namespace esphome;

/**
 * Create a new MitsubishiHeatPump object
 *
 * Args:
 *   hw_serial: pointer to an Arduino HardwareSerial instance
 *   poll_interval: polling interval in milliseconds
 */
MitsubishiHeatPump::MitsubishiHeatPump(
        HardwareSerial* hw_serial,
        uint32_t poll_interval
) :
    PollingComponent{poll_interval}, // member initializers list
    hw_serial_{hw_serial}
{ }

void MitsubishiHeatPump::check_logger_conflict_() {
#ifdef USE_LOGGER
    if (this->get_hw_serial_() == logger::global_logger->get_hw_serial()) {
        ESP_LOGW(TAG, "  You're using the same serial port for logging"
                " and the MitsubishiHeatPump component. Please disable"
                " logging over the serial port by setting"
                " logger:baud_rate to 0.");
    }
#endif
}

void MitsubishiHeatPump::update() {
    // This will be called every "update_interval" milliseconds.
    //this->dump_config();
    this->hp->sync();
#ifndef USE_CALLBACKS
    this->hpSettingsChanged();
    heatpumpStatus currentStatus = hp->getStatus();
    this->hpStatusChanged(currentStatus);
#endif
}

/**
 * Define our supported traits.
 *
 * Note:
 * Many of the following traits are only available in the 1.5.0 dev train of
 * ESPHome, particularly the Dry operation mode, and several of the fan modes.
 *
 * Returns:
 *   This class' supported climate::ClimateTraits.
 */
climate::ClimateTraits MitsubishiHeatPump::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supports_action(true);
    traits.set_supports_current_temperature(true);
    traits.set_supports_auto_mode(true);
    traits.set_supports_cool_mode(true);
    traits.set_supports_heat_mode(true);
    traits.set_supports_dry_mode(true);
    traits.set_supports_fan_only_mode(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_away(false);
    traits.set_visual_min_temperature(ESPMHP_MIN_TEMPERATURE);
    traits.set_visual_max_temperature(ESPMHP_MAX_TEMPERATURE);
    traits.set_visual_temperature_step(ESPMHP_TEMPERATURE_STEP);
    traits.set_supports_fan_mode_on(false);
    traits.set_supports_fan_mode_off(false);
    traits.set_supports_fan_mode_auto(true);
    traits.set_supports_fan_mode_focus(false);
    traits.set_supports_fan_mode_diffuse(true);
    traits.set_supports_fan_mode_low(true);
    traits.set_supports_fan_mode_medium(true);
    traits.set_supports_fan_mode_middle(true);
    traits.set_supports_fan_mode_high(true);
    traits.set_supports_swing_mode_off(true);
    traits.set_supports_swing_mode_both(false);
    traits.set_supports_swing_mode_vertical(true);
    traits.set_supports_swing_mode_horizontal(false);
    return traits;
}

/**
 * Implement control of a MitsubishiHeatPump.
 *
 * Maps HomeAssistant/ESPHome modes to Mitsubishi modes.
 */
void MitsubishiHeatPump::control(const climate::ClimateCall &call) {
    ESP_LOGV(TAG, "Control called.");

    bool updated = false;
    if (call.get_mode().has_value()){
        this->mode = *call.get_mode();
        switch (*call.get_mode()) {
            case climate::CLIMATE_MODE_COOL:
                hp->setModeSetting("COOL");
                hp->setPowerSetting("ON");
                updated = true;
                break;
            case climate::CLIMATE_MODE_HEAT:
                hp->setModeSetting("HEAT");
                hp->setPowerSetting("ON");
                updated = true;
                break;
            case climate::CLIMATE_MODE_DRY:
                hp->setModeSetting("DRY");
                hp->setPowerSetting("ON");
                updated = true;
                break;
            case climate::CLIMATE_MODE_AUTO:
                hp->setModeSetting("AUTO");
                hp->setPowerSetting("ON");
                updated = true;
                break;
            case climate::CLIMATE_MODE_FAN_ONLY:
                hp->setModeSetting("FAN");
                hp->setPowerSetting("ON");
                updated = true;
                break;
            case climate::CLIMATE_MODE_OFF:
            default:
                hp->setPowerSetting("OFF");
                updated = true;
                break;
        }
    }

    if (call.get_target_temperature().has_value()){
        ESP_LOGV(
            "control", "Sending target temp: %.1f",
            *call.get_target_temperature()
        )
        hp->setTemperature(*call.get_target_temperature());
        this->target_temperature = *call.get_target_temperature();
        updated = true;
    }

    //const char* FAN_MAP[6]         = {"AUTO", "QUIET", "1", "2", "3", "4"};
    if (call.get_fan_mode().has_value()) {
        ESP_LOGV("control", "Requested fan mode is %s", *call.get_fan_mode());
        this->fan_mode = *call.get_fan_mode();
        switch(*call.get_fan_mode()) {
            case climate::CLIMATE_FAN_OFF:
                hp->setPowerSetting("OFF");
                updated = true;
                break;
            case climate::CLIMATE_FAN_DIFFUSE:
                hp->setFanSpeed("QUIET");
                updated = true;
                break;
            case climate::CLIMATE_FAN_LOW:
                hp->setFanSpeed("1");
                updated = true;
                break;
            case climate::CLIMATE_FAN_MEDIUM:
                hp->setFanSpeed("2");
                updated = true;
                break;
            case climate::CLIMATE_FAN_MIDDLE:
                hp->setFanSpeed("3");
                updated = true;
                break;
            case climate::CLIMATE_FAN_HIGH:
                hp->setFanSpeed("4");
                updated = true;
                break;
            case climate::CLIMATE_FAN_ON:
            case climate::CLIMATE_FAN_AUTO:
            default:
                hp->setFanSpeed("AUTO");
                updated = true;
                break;
        }
    }

    //const char* VANE_MAP[7]        = {"AUTO", "1", "2", "3", "4", "5", "SWING"};
    if (call.get_swing_mode().has_value()) {
        ESP_LOGV(TAG, "control - requested swing mode is %s",
                *call.get_swing_mode());

        this->swing_mode = *call.get_swing_mode();
        switch(*call.get_swing_mode()) {
            case climate::CLIMATE_SWING_OFF:
                hp->setVaneSetting("AUTO");
                updated = true;
                break;
            case climate::CLIMATE_SWING_VERTICAL:
                hp->setVaneSetting("SWING");
                updated = true;
                break;
            default:
                ESP_LOGW(TAG, "control - received unsupported swing mode request.");

        }
    }
    ESP_LOGD(TAG, "control - Was HeatPump updated? %s", YESNO(updated));

    // send the update back to esphome:
    this->publish_state();
    // and the heat pump:
    hp->update();
}

void MitsubishiHeatPump::hpSettingsChanged() {
    heatpumpSettings currentSettings = hp->getSettings();

    if (currentSettings.power == NULL) {
        /*
         * We should always get a valid pointer here once the HeatPump
         * component fully initializes. If HeatPump hasn't read the settings
         * from the unit yet (hp->connect() doesn't do this, sadly), we'll need
         * to punt on the update. Likely not an issue when run in callback
         * mode, but that isn't working right yet.
         */
        ESP_LOGW(TAG, "Waiting for HeatPump to read the settings the first time.");
        delay(10);
        return;
    }

    /*
     * ************ HANDLE POWER AND MODE CHANGES ***********
     * https://github.com/geoffdavis/HeatPump/blob/stream/src/HeatPump.h#L125
     * const char* POWER_MAP[2]       = {"OFF", "ON"};
     * const char* MODE_MAP[5]        = {"HEAT", "DRY", "COOL", "FAN", "AUTO"};
     */
    if (strcmp(currentSettings.power, "ON") == 0) {
        if (strcmp(currentSettings.mode, "HEAT") == 0) {
            this->mode = climate::CLIMATE_MODE_HEAT;
        } else if (strcmp(currentSettings.mode, "DRY") == 0) {
            this->mode = climate::CLIMATE_MODE_DRY;
        } else if (strcmp(currentSettings.mode, "COOL") == 0) {
            this->mode = climate::CLIMATE_MODE_COOL;
        } else if (strcmp(currentSettings.mode, "FAN") == 0) {
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        } else if (strcmp(currentSettings.mode, "AUTO") == 0) {
            this->mode = climate::CLIMATE_MODE_AUTO;
        } else {
            ESP_LOGW(
                    TAG,
                    "Unknown climate mode value %s received from HeatPump",
                    currentSettings.mode
            );
        }
    } else {
        this->mode = climate::CLIMATE_MODE_OFF;
    }

    ESP_LOGI(TAG, "Climate mode is: %i", this->mode);

    /*
     * ******* HANDLE FAN CHANGES ********
     *
     * const char* FAN_MAP[6]         = {"AUTO", "QUIET", "1", "2", "3", "4"};
     */
    if (strcmp(currentSettings.fan, "QUIET") == 0) {
        this->fan_mode = climate::CLIMATE_FAN_DIFFUSE;
    } else if (strcmp(currentSettings.fan, "1") == 0) {
            this->fan_mode = climate::CLIMATE_FAN_LOW;
    } else if (strcmp(currentSettings.fan, "2") == 0) {
            this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
    } else if (strcmp(currentSettings.fan, "3") == 0) {
            this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
    } else if (strcmp(currentSettings.fan, "4") == 0) {
            this->fan_mode = climate::CLIMATE_FAN_HIGH;
    } else { //case "AUTO" or default:
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
    }
    ESP_LOGI(TAG, "Fan mode is: %i", this->fan_mode);

    /* ******** HANDLE MITSUBISHI VANE CHANGES ********
     * const char* VANE_MAP[7]        = {"AUTO", "1", "2", "3", "4", "5", "SWING"};
     */
    if (
            (strcmp(currentSettings.vane, "AUTO") == 0)
            || (strcmp(currentSettings.vane, "SWING") == 0)
    ) {
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    }
    else {
        this->swing_mode = climate::CLIMATE_SWING_OFF;
    }
    ESP_LOGI(TAG, "Swing mode is: %i", this->swing_mode);



    /*
     * ******** HANDLE TARGET TEMPERATURE CHANGES ********
     */
    this->target_temperature = currentSettings.temperature;
    ESP_LOGI(TAG, "Target temp is: %f", this->target_temperature);


    /*
     * Compute running state from mode & temperatures
     */
    switch (this->mode) {
        case climate::CLIMATE_MODE_HEAT:
            if (this->current_temperature < this->target_temperature) {
                this->action = climate::CLIMATE_ACTION_HEATING;
            }
            else {
                this->action = climate::CLIMATE_ACTION_IDLE;
            }
            break;
        case climate::CLIMATE_MODE_COOL:
            if (this->current_temperature > this->target_temperature) {
                this->action = climate::CLIMATE_ACTION_COOLING;
            }
            else {
                this->action = climate::CLIMATE_ACTION_IDLE;
            }
            break;
        case climate::CLIMATE_MODE_DRY:
            this->action = climate::CLIMATE_ACTION_DRYING;
        default:
            this->action = climate::CLIMATE_ACTION_OFF;
    }

    /*
     * ******** Publish state back to ESPHome. ********
     */
    this->publish_state();
}

/**
 * Report changes in the current temperature sensed by the HeatPump.
 */
void MitsubishiHeatPump::hpStatusChanged(heatpumpStatus currentStatus) {
    this->current_temperature = currentStatus.roomTemperature;
    this->publish_state();
}

void MitsubishiHeatPump::setup() {
    // This will be called by App.setup()
    this->banner();
    ESP_LOGCONFIG(TAG, "Setting up UART...");
    if (!this->get_hw_serial_()) {
        ESP_LOGCONFIG(
                TAG,
                "No HardwareSerial was provided. "
                "Software serial ports are unsupported by this component."
        );
        this->mark_failed();
        return;
    }
    this->check_logger_conflict_();

    ESP_LOGCONFIG(TAG, "Intializing new HeatPump object.");
    this->hp = new HeatPump();

#ifdef USE_CALLBACKS
    hp->setSettingsChangedCallback(
            [this]() {
                this->hpSettingsChanged();
            }
    );

    hp->setStatusChangedCallback(
            [this](heatpumpStatus currentStatus) {
                this->hpStatusChanged(currentStatus);
            }
    );
#endif

    ESP_LOGCONFIG(
            TAG,
            "hw_serial(%p) is &Serial(%p)? %s",
            this->get_hw_serial_(),
            &Serial,
            YESNO(this->get_hw_serial_() == &Serial)
    );

    ESP_LOGCONFIG(TAG, "Calling hp->connect(%p)", this->get_hw_serial_());

    if (hp->connect(this->get_hw_serial_())) {
        hp->sync();
    }
    else {
        ESP_LOGCONFIG(
                TAG,
                "Connection to HeatPump failed."
                " Marking MitsubishiHeatPump component as failed."
        );
        this->mark_failed();
    }

    this->dump_config();
}

void MitsubishiHeatPump::dump_config() {
    this->banner();
    ESP_LOGI(TAG, "  Supports HEAT: %s", YESNO(true));
    ESP_LOGI(TAG, "  Supports COOL: %s", YESNO(true));
    ESP_LOGI(TAG, "  Supports AWAY mode: %s", YESNO(false));
}

void MitsubishiHeatPump::dump_state() {
    LOG_CLIMATE("", "MitsubishiHeatPump Climate", this);
    ESP_LOGI(TAG, "HELLO");
}
