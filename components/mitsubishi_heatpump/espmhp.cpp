/**
 * espmhp.cpp
 *
 * Implementation of esphome-mitsubishiheatpump
 *
 * Author: Geoff Davis <geoff@geoffdavis.com>
 * Author: Phil Genera @pgenera on Github.
 * Author: Barry Loong @loongyh on GitHub.
 * Author: @am-io on Github.
 * Author: @nao-pon on Github.
 * Author: Simon Knopp @sijk on Github
 * Last Updated: 2021-05-27
 * License: BSD
 *
 * Requirements:
 * - https://github.com/SwiCago/HeatPump
 * - ESPHome 1.18.0 or greater
 */

#include "espmhp.h"
using namespace esphome;


#ifdef ESP32
#include <WiFi.h>
#endif

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
    PollingComponent{ poll_interval }, // member initializers list
    hw_serial_{ hw_serial } {
    this->traits_.set_supports_action(true);
    this->traits_.set_supports_current_temperature(true);
    this->traits_.set_supports_two_point_target_temperature(false);
    this->traits_.set_visual_min_temperature(ESPMHP_MIN_TEMPERATURE);
    this->traits_.set_visual_max_temperature(ESPMHP_MAX_TEMPERATURE);
    this->traits_.set_visual_temperature_step(ESPMHP_TEMPERATURE_STEP);
}

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
    ESP_LOGD(TAG, "update() called");
    this->hp->sync();

#ifndef USE_CALLBACKS
    this->hpSettingsChanged();
    heatpumpStatus currentStatus = hp->getStatus();
    this->hpStatusChanged(currentStatus);
#endif
    this->dump_state();
}

void MitsubishiHeatPump::set_baud_rate(int baud) {
    this->baud_ = baud;
}

/**
 * Get our supported traits.
 *
 * Note:
 * Many of the following traits are only available in the 1.5.0 dev train of
 * ESPHome, particularly the Dry operation mode, and several of the fan modes.
 *
 * Returns:
 *   This class' supported climate::ClimateTraits.
 */
climate::ClimateTraits MitsubishiHeatPump::traits() {
    return traits_;
}

/**
 * Modify our supported traits.
 *
 * Returns:
 *   A reference to this class' supported climate::ClimateTraits.
 */
climate::ClimateTraits& MitsubishiHeatPump::config_traits() {
    return traits_;
}

/**
 * Implement control of a MitsubishiHeatPump.
 *
 * Maps HomeAssistant/ESPHome modes to Mitsubishi modes.
 */
void MitsubishiHeatPump::control(const climate::ClimateCall& call) {
    ESP_LOGV(TAG, "Control called.");

    bool updated = false;
    bool has_mode = call.get_mode().has_value();
    bool has_temp = call.get_target_temperature().has_value();
    if (has_mode) {
        this->mode = *call.get_mode();
    }
    switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
        hp->setModeSetting("COOL");
        hp->setPowerSetting("ON");

        if (has_mode) {
            if (cool_setpoint.has_value() && !has_temp) {
                hp->setTemperature(cool_setpoint.value());
                this->target_temperature = cool_setpoint.value();
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
            updated = true;
        }
        break;
    case climate::CLIMATE_MODE_HEAT:
        hp->setModeSetting("HEAT");
        hp->setPowerSetting("ON");
        if (has_mode) {
            if (heat_setpoint.has_value() && !has_temp) {
                hp->setTemperature(heat_setpoint.value());
                this->target_temperature = heat_setpoint.value();
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
            updated = true;
        }
        break;
    case climate::CLIMATE_MODE_DRY:
        hp->setModeSetting("DRY");
        hp->setPowerSetting("ON");
        if (has_mode) {
            this->action = climate::CLIMATE_ACTION_DRYING;
            updated = true;
        }
        break;
    case climate::CLIMATE_MODE_HEAT_COOL:
        hp->setModeSetting("AUTO");
        hp->setPowerSetting("ON");
        if (has_mode) {
            if (auto_setpoint.has_value() && !has_temp) {
                hp->setTemperature(auto_setpoint.value());
                this->target_temperature = auto_setpoint.value();
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
        }
        updated = true;
        break;
    case climate::CLIMATE_MODE_FAN_ONLY:
        hp->setModeSetting("FAN");
        hp->setPowerSetting("ON");
        if (has_mode) {
            this->action = climate::CLIMATE_ACTION_FAN;
            updated = true;
        }
        break;
    case climate::CLIMATE_MODE_OFF:
    default:
        if (has_mode) {
            hp->setPowerSetting("OFF");
            this->action = climate::CLIMATE_ACTION_OFF;
            updated = true;
        }
        break;
    }

    if (has_temp) {
        ESP_LOGV(
            "control", "Sending target temp: %.1f",
            *call.get_target_temperature()
        );
        hp->setTemperature(*call.get_target_temperature());
        this->target_temperature = *call.get_target_temperature();
        updated = true;
    }

    //const char* FAN_MAP[6]         = {"AUTO", "QUIET", "1", "2", "3", "4"};
    if (call.get_fan_mode().has_value()) {
        ESP_LOGV("control", "Requested fan mode is %s", *call.get_fan_mode());
        this->fan_mode = *call.get_fan_mode();
        switch (*call.get_fan_mode()) {
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
        switch (*call.get_swing_mode()) {
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
        ESP_LOGW(TAG, "Waiting for HeatPump to read the settings the first time. %d", this->cpt_);
        esphome::delay(10);

        /*if (this->cpt_ == 50) {
            ESP_LOGW(TAG, "50 tries, calling setup() again.");
            this->component_state_ = 0;
            this->cpt_++;
            this->setup();
            // this->cpt_ = 0;
        }
        this->cpt_++;*/

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
            if (heat_setpoint != currentSettings.temperature) {
                heat_setpoint = currentSettings.temperature;
                save(currentSettings.temperature, heat_storage);
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
        } else if (strcmp(currentSettings.mode, "DRY") == 0) {
            this->mode = climate::CLIMATE_MODE_DRY;
            this->action = climate::CLIMATE_ACTION_DRYING;
        } else if (strcmp(currentSettings.mode, "COOL") == 0) {
            this->mode = climate::CLIMATE_MODE_COOL;
            if (cool_setpoint != currentSettings.temperature) {
                cool_setpoint = currentSettings.temperature;
                save(currentSettings.temperature, cool_storage);
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
        } else if (strcmp(currentSettings.mode, "FAN") == 0) {
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            this->action = climate::CLIMATE_ACTION_FAN;
        } else if (strcmp(currentSettings.mode, "AUTO") == 0) {
            this->mode = climate::CLIMATE_MODE_HEAT_COOL;
            if (auto_setpoint != currentSettings.temperature) {
                auto_setpoint = currentSettings.temperature;
                save(currentSettings.temperature, auto_storage);
            }
            this->action = climate::CLIMATE_ACTION_IDLE;
        } else {
            ESP_LOGW(
                TAG,
                "Unknown climate mode value %s received from HeatPump",
                currentSettings.mode
            );
        }
    } else {
        this->mode = climate::CLIMATE_MODE_OFF;
        this->action = climate::CLIMATE_ACTION_OFF;
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
    if (strcmp(currentSettings.vane, "SWING") == 0) {
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    } else {
        this->swing_mode = climate::CLIMATE_SWING_OFF;
    }
    ESP_LOGI(TAG, "Swing mode is: %i", this->swing_mode);



    /*
     * ******** HANDLE TARGET TEMPERATURE CHANGES ********
     */
    this->target_temperature = currentSettings.temperature;
    ESP_LOGI(TAG, "Target temp is: %f", this->target_temperature);

    /*
     * ******** Publish state back to ESPHome. ********
     */
    this->publish_state();
}
void MitsubishiHeatPump::hpDidConnect() {
    ESP_LOGD(TAG, "Connection à la clim établie");
}

void MitsubishiHeatPump::hpPacketDebug(byte* packet, unsigned int length, const char* packetDirection) {
    String message;
    for (unsigned int idx = 0; idx < length; idx++) {
        if (packet[idx] < 16) {
            message += "0"; // pad single hex digits with a 0
        }
        message += String(packet[idx], HEX) + " ";
    }
    ESP_LOGD(TAG, "hpPacketDebug %s: %s", packetDirection, message);

}

void MitsubishiHeatPump::hpRoomTempChanged(float currentRoomTemperature) {
    ESP_LOGD(TAG, "roomTempChanged %f", currentRoomTemperature);
    this->current_temperature = currentRoomTemperature;
    this->dump_state();
}

/**
 * Report changes in the current temperature sensed by the HeatPump.
 */
void MitsubishiHeatPump::hpStatusChanged(heatpumpStatus currentStatus) {
    ESP_LOGD(TAG, "hpStatusChanged", currentStatus);
    this->current_temperature = currentStatus.roomTemperature;
    ESP_LOGD(TAG, "hpStatusChanged t°", currentStatus.roomTemperature);
    switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT:
        if (currentStatus.operating) {
            this->action = climate::CLIMATE_ACTION_HEATING;
        } else {
            this->action = climate::CLIMATE_ACTION_IDLE;
        }
        break;
    case climate::CLIMATE_MODE_COOL:
        if (currentStatus.operating) {
            this->action = climate::CLIMATE_ACTION_COOLING;
        } else {
            this->action = climate::CLIMATE_ACTION_IDLE;
        }
        break;
    case climate::CLIMATE_MODE_HEAT_COOL:
        this->action = climate::CLIMATE_ACTION_IDLE;
        if (currentStatus.operating) {
            if (this->current_temperature > this->target_temperature) {
                this->action = climate::CLIMATE_ACTION_COOLING;
            } else if (this->current_temperature < this->target_temperature) {
                this->action = climate::CLIMATE_ACTION_HEATING;
            }
        }
        break;
    case climate::CLIMATE_MODE_DRY:
        if (currentStatus.operating) {
            this->action = climate::CLIMATE_ACTION_DRYING;
        } else {
            this->action = climate::CLIMATE_ACTION_IDLE;
        }
        break;
    case climate::CLIMATE_MODE_FAN_ONLY:
        this->action = climate::CLIMATE_ACTION_FAN;
        break;
    default:
        this->action = climate::CLIMATE_ACTION_OFF;
    }

    this->publish_state();
}

void MitsubishiHeatPump::set_remote_temperature(float temp) {
    ESP_LOGD(TAG, "Setting remote temp: %.1f", temp);
    this->hp->setRemoteTemperature(temp);
}

void MitsubishiHeatPump::setup() {


    ESP_LOGD(TAG, "SETUP starting...");

    // This will be called by App.setup()
    this->banner();
    ESP_LOGD(TAG, "Setting up UART...");
    if (!this->get_hw_serial_()) {
        ESP_LOGW(TAG, "No HardwareSerial was provided. Software serial ports are unsupported by this component.");
        this->mark_failed();
        return;
    }
    this->check_logger_conflict_();

    ESP_LOGD(TAG, "Intializing new HeatPump object.");

    this->hp = new HeatPump();
    //hp->enableAutoUpdate();
    hp->enableExternalUpdate(); // does enable autoUpdate too

    this->current_temperature = NAN;
    this->target_temperature = NAN;
    this->fan_mode = climate::CLIMATE_FAN_OFF;
    this->swing_mode = climate::CLIMATE_SWING_OFF;

#ifdef USE_CALLBACKS
    hp->setSettingsChangedCallback(
        [this]() {
            this->hpSettingsChanged();
        }
    );
    ESP_LOGD(TAG, "Setting callback onConnect changed function...");
    hp->setOnConnectCallback(
        [this]() {
            this->hpDidConnect();
        }
    );
    ESP_LOGD(TAG, "Setting callback status changed function...");
    hp->setStatusChangedCallback(
        [this](heatpumpStatus currentStatus) {
            this->hpStatusChanged(currentStatus);
        }
    );
    hp->setRoomTempChangedCallback(
        [this](float currentRoomTemperature) {
            this->hpRoomTempChanged(currentRoomTemperature);
        }
    );

    hp->setPacketCallback(
        [this](byte* packet, unsigned int length, const char* packetDirection) {
            this->hpPacketDebug(packet, length, packetDirection);
        }
    );
#endif

    ESP_LOGD(
        TAG,
        "hw_serial(%p) is &Serial(%p)? %s",
        this->get_hw_serial_(),
        &Serial,
        YESNO(this->get_hw_serial_() == &Serial)
    );

    ESP_LOGD(TAG, "bauds: %d", this->baud_);

    ESP_LOGD(TAG, "delaying 10 sec...");
    esphome::delay(10000);

    ESP_LOGD(TAG, "Calling hp->connect(%p)", this->get_hw_serial_());

    //hp->connect(this->get_hw_serial_(), this->baud_);
    int nb = 1;
    bool success = false;
    while ((!(success = hp->connect(this->get_hw_serial_(), this->baud_, -1, -1)) && (nb < 6))) {
        ESP_LOGD(TAG, "test %d delaying 5 sec...", ++nb);
        ESP_LOGD(TAG, "Calling again hp->connect(%p)", this->get_hw_serial_());
        esphome::delay(5000);
    }
    if (success) {
        ESP_LOGD(TAG, "delaying 2 sec...");
        esphome::delay(2000);
        hp->sync();
    } else {
        ESP_LOGE(
            TAG,
            "Connection to HeatPump failed."
            " Marking MitsubishiHeatPump component as failed."
        );
        this->mark_failed();
    }

    /*if (hp->connect(this->get_hw_serial_(), this->baud_, -1, -1)) {
        ESP_LOGD(TAG, "delaying 6 sec...");
        esphome::delay(6000);
        hp->sync();
    } else {
        ESP_LOGE(
            TAG,
            "Connection to HeatPump failed."
            //" Marking MitsubishiHeatPump component as failed."
        );
        //this->mark_failed();
    }*/

    // create various setpoint persistence:
    cool_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 1);
    heat_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 2);
    auto_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 3);

    // load values from storage:
    cool_setpoint = load(cool_storage);
    heat_setpoint = load(heat_storage);
    auto_setpoint = load(auto_storage);

    ESP_LOGI(TAG, "calling dump config:");

    this->dump_config();
}

/**
 * The ESP only has a few bytes of rtc storage, so instead
 * of storing floats directly, we'll store the number of
 * TEMPERATURE_STEPs from MIN_TEMPERATURE.
 **/
void MitsubishiHeatPump::save(float value, ESPPreferenceObject& storage) {
    uint8_t steps = (value - ESPMHP_MIN_TEMPERATURE) / ESPMHP_TEMPERATURE_STEP;
    storage.save(&steps);
}

optional<float> MitsubishiHeatPump::load(ESPPreferenceObject& storage) {
    uint8_t steps = 0;
    if (!storage.load(&steps)) {
        return {};
    }
    return ESPMHP_MIN_TEMPERATURE + (steps * ESPMHP_TEMPERATURE_STEP);
}

void MitsubishiHeatPump::dump_config() {
    this->banner();
    ESP_LOGI(TAG, "  Version de Eric Chavet 1.0");
    ESP_LOGI(TAG, "  Supports HEAT: %s", YESNO(true));
    ESP_LOGI(TAG, "  Supports COOL: %s", YESNO(true));
    ESP_LOGI(TAG, "  Supports AWAY mode: %s", YESNO(false));
    ESP_LOGI(TAG, "  Saved heat: %.1f", heat_setpoint.value_or(-1));
    ESP_LOGI(TAG, "  Saved cool: %.1f", cool_setpoint.value_or(-1));
    ESP_LOGI(TAG, "  Saved auto: %.1f", auto_setpoint.value_or(-1));
    ESP_LOGI(TAG, "  ---- ");
    ESP_LOGI(TAG, "  hp : ", this->hp);
    ESP_LOGI(TAG, "heatpump connected:", this->hp->isConnected());
    ESP_LOGD(TAG, "heatpump room temp: %f", this->hp->getRoomTemperature());
    ESP_LOGI(TAG, "heatpump room temp: %f", this->hp->getRoomTemperature());
    ESP_LOGD(TAG, "heatpump operating: ", this->hp->getOperating());
    ESP_LOGI(TAG, "  ---- ");

#ifndef USE_CALLBACKS
    ESP_LOGI(TAG, "Not using callbacks");
#else
    ESP_LOGI(TAG, "Using callbacks");
#endif

}

void MitsubishiHeatPump::dump_state() {

    ESP_LOGI(TAG, "HELLO from echavet");
    LOG_CLIMATE("", "MitsubishiHeatPump Climate", this);

    heatpumpSettings settings = this->hp->getSettings();
    heatpumpStatus status = this->hp->getStatus();

    ESP_LOGI(TAG, "HP settings:");
    if (settings.power != NULL) {
        ESP_LOGD(TAG, "power: %s", settings.power);
        ESP_LOGD(TAG, "fan: %s", settings.fan);
        ESP_LOGD(TAG, "temperature: %f", settings.temperature);
        ESP_LOGD(TAG, "mode: %s", settings.mode);
    } else {
        ESP_LOGE(TAG, "settings.power est NULL");
    }

    ESP_LOGI(TAG, "HP status:");

    ESP_LOGD(TAG, "operating: %d", status.operating);
    ESP_LOGD(TAG, "room temp : %f", status.roomTemperature);

    ESP_LOGI(TAG, "  component state : %d", this->component_state_);

}
