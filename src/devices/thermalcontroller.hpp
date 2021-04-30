#ifndef THERMALCONTROLLER_H_
#define THERMALCONTROLLER_H_

#include <stdint.h>

#ifdef _DEBUG
#include "../../tests/tests/thermalcontrollermocks.h"
#else
#include "fan.h"
#include "led_driver.h"
#include "thermosensors.hpp"
#endif

class ThermalController
{
public:
    ThermalController(ThermoSensors& thermo_sensors, FanPWM& fan, LedDriver& led_driver);
    void Loop();

private:
    void AdjustFanSpeed(float temperature);
    void AdjustTemperatureFactor(float temperature);

    ThermoSensors& thermo_sensors_;
    FanPWM&        fan_;
    LedDriver&     led_driver_;

    uint8_t last_fan_speed_;
    bool    is_max_fan_speed_enabled_;
};

#endif  // THERMALCONTROLLER_H_
