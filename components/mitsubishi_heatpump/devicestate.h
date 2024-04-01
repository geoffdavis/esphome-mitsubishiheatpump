#include "HeatPump.h"

#ifndef DEVICESTATE_H
#define DEVICESTATE_H

namespace devicestate {
  enum DeviceMode {
    Off,
    Heat,
    Cool,
    Dry,
    Fan,
    Auto,
    Unknown
  };

  struct DeviceState {
    bool active;
    DeviceMode mode;
    float targetTemperature;
  };

  DeviceState toDeviceState(heatpumpSettings *currentSettings);
}
#endif