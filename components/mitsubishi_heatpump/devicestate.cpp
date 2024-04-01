#include "devicestate.h"

namespace devicestate {
    bool isDeviceActive(heatpumpSettings *currentSettings) {
        return strcmp(currentSettings->power, "ON") == 0;
    }

    DeviceMode toDeviceMode(heatpumpSettings *currentSettings) {
        if (strcmp(currentSettings->mode, "HEAT") == 0) {
            return DeviceMode::Heat;
        } else if (strcmp(currentSettings->mode, "DRY") == 0) {
            return DeviceMode::Dry;
        } else if (strcmp(currentSettings->mode, "COOL") == 0) {
        return DeviceMode::Cool;
        } else if (strcmp(currentSettings->mode, "FAN") == 0) {
            return DeviceMode::Fan;
        } else if (strcmp(currentSettings->mode, "AUTO") == 0) {
            return DeviceMode::Auto;
        } else {
            return DeviceMode::Unknown;
        }
    }

    DeviceState toDeviceState(heatpumpSettings *currentSettings) {
        DeviceState deviceState;
        deviceState.active = isDeviceActive(currentSettings);
        deviceState.mode = toDeviceMode(currentSettings);
        deviceState.targetTemperature = currentSettings->temperature;
        return deviceState;
    }
}