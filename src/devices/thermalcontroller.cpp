#include "thermalcontroller.hpp"

#ifndef _DEBUG
#include <Arduino.h>
#include "../Utils.h"
#endif

namespace
{
constexpr uint8_t kNumOfSensors{2};
struct TempGraphPoint
{
    uint8_t temperature;
    uint8_t speed;
};
/*
 * NOTE! After implementing gradual transitions, graph doesn't look like ledders anymore. Instead, there are smooth
 *       change of PWM between specified poits. Fan PWM (before gradual transitions)
 *     ^
 * 100 |                                         **********************
 * 90  |                                  *******
 * 80  |
 * 70  |
 * 60  |                           *******
 * 50  |
 * 40  |
 * 30  |                    *******
 * 20  |
 * 10  |             *******
 * 0   |*************
 *     L---------------------------------------------------------------> Temperature
 *           20     30     40     50     60     70     80     90     100
 */
// In graph last point should always contain speed 255
// TODO1: temp sensor is quite isolated from heatsink by glue. Also it is quite far from LED. So, I would set shutdown
// temperature to 75, max temperature on graph to 65
// TODO2: uncomment after debugging is finished
// constexpr TempGraphPoint temperature_graph[] = {{30.0, 25}, {40.0, 76}, {50.0, 153}, {60.0, 230}, {70.0,
// 255}};
// NOTE: storing this aggay in PROGMEM will decrease flash size on 200 bytes, but will give only 24 bytes of ram.
// Moreover to read this data from PROGMEM you will have to spend more RAM at runtime (you will need to read 2
// structures TempGraphPoint).
constexpr TempGraphPoint temperature_graph[] = {{27, 25}, {29, 76}, {35, 153}, {40, 230}, {45, 255}};
constexpr uint8_t        kNumOfTempgraphLevels{sizeof(temperature_graph) / sizeof(temperature_graph[0])};
constexpr uint8_t        kMaxTemperature{temperature_graph[kNumOfTempgraphLevels - 1].temperature};
constexpr uint8_t        kShutDownTemperatureRange{10};
constexpr uint32_t       kControllTimeout{1000};

// TODO: make kShutDownTemperatureRange and kNumOfTempgraphLevels configurable via WebUI

// For given temperature this function should check temperature_graph and get appropriate temperature
uint8_t
MapTemperatureToFanSpeed(float temperature)
{
    // Edge cases
    if (temperature < temperature_graph[0].temperature) {
        return 0;
    }
    else if (temperature >= kMaxTemperature) {
        return 255;
    }

    for (uint8_t current_index = 0; current_index < (kNumOfTempgraphLevels - 1); ++current_index) {
        // NOTE! It doesn't worth storing temperature_graph in PROGMEM. But in case you will decide to do it, use
        // following code to read data from it.
        // TempGraphPoint current_point;
        // TempGraphPoint next_point;
        // memcpy_P(&current_point, &temperature_graph[current_index], sizeof(TempGraphPoint));
        // memcpy_P(&next_point, &temperature_graph[current_index + 1], sizeof(TempGraphPoint));

        if ((temperature_graph[current_index].temperature <= temperature) &&
            (temperature < temperature_graph[current_index + 1].temperature)) {
            // Make gradual transitions for speed.
            const uint8_t current_range_speed       = temperature_graph[current_index].speed;
            const uint8_t next_range_speed          = temperature_graph[current_index + 1].speed;
            const uint8_t current_range_temperature = temperature_graph[current_index].temperature;
            const uint8_t next_range_temperature    = temperature_graph[current_index + 1].temperature;
            return current_range_speed +
                   static_cast<uint8_t>((float)(next_range_speed - current_range_speed) *
                                        ((float)(temperature - (float)current_range_temperature) /
                                         (float)(next_range_temperature - current_range_temperature)));
        }
    }

    return 255;
}

}  // namespace

ThermalController::ThermalController(ThermoSensors& thermo_sensors, FanPWM& fan, LedDriver& led_driver)
  : thermo_sensors_{thermo_sensors}
  , fan_{fan}
  , led_driver_{led_driver}
  , last_fan_speed_{0}
  , is_max_fan_speed_enabled_{false}
{
}

void
ThermalController::Loop()
{
    // Run controll logic only once per kControllTimeout milliseconds
    static uint32_t last_controll_time{0};
    auto            now = millis();
    if ((now - last_controll_time) < kControllTimeout) {
        return;
    }
    last_controll_time = now;

    float temperatures[kNumOfSensors];
    thermo_sensors_.GetTemperatures(temperatures);
    if (temperatures[0] == ThermoSensors::kInvalidTemperature) {
        Serial.println(String(F("Error: Could not read temperature data from sensor ")) + String(0));
    }
    if (temperatures[1] == ThermoSensors::kInvalidTemperature) {
        Serial.println(String(F("Error: Could not read temperature data from sensor ")) + String(1));
    }
    auto max_current_temp = max(temperatures[0], temperatures[1]);

    Serial.println(String(F("LAMBIN max_current_temp = ")) + String(max_current_temp, 2));

    AdjustFanSpeed(max_current_temp);
    AdjustTemperatureFactor(max_current_temp);
}

void
ThermalController::AdjustFanSpeed(float temperature)
{
    if (is_max_fan_speed_enabled_) {
        // If we are in max fan speed mode, no need to calculate speed again.
        // Skip expensive calculations with float point
        return;
    }

    auto fan_speed = MapTemperatureToFanSpeed(temperature);
    if (fan_speed != last_fan_speed_) {
        last_fan_speed_ = fan_speed;
        fan_.SetSpeed(fan_speed);
        if (fan_speed == 255) {
            is_max_fan_speed_enabled_ = true;
        }
    }
}
void
ThermalController::AdjustTemperatureFactor(float temperature)
{
    if (temperature >= (kMaxTemperature + kShutDownTemperatureRange)) {
        led_driver_.SetThermalFactor(0.0);
    }
    else if (temperature >= kMaxTemperature) {
        // Both calcualtions are the same, but 2nd is faster
        // led_driver_.SetThermalFactor(1.0F - (temperature - kMaxTemperature) / kShutDownTemperatureRange);
        constexpr uint8_t kShutDownTemperature{kShutDownTemperatureRange + kMaxTemperature};
        led_driver_.SetThermalFactor(((float)kShutDownTemperature - temperature) / (float)kShutDownTemperatureRange);


        // TODO: in case fans are running on 100% but temperature is still too hot, we should reduce power of
        // LedDriver. Ex. we can call LedDriver::set_thermal_factor(float t) and LedDriver will set its brightness
        // as "level * thremal_factor"? But how to check fact, that we are on 100% fan power for some time and
        // temperature doesn't go down? should we track time here, for which PWM is 100% ? How to gradually reduce
        // thermal_factor to let brightness not jump, but change smoothly?
        //
        // May be google "LED temperature controll" ?
        //
        // Idea: if T > 80, k = 0
        //       else k = 1 - (T - 70)/10
        //       It can be evaluater immediately, without waiting for some pause on PWM=100%.
        // Ex. PWM == 100%, T = 70. So, k = 1. If fans are good enough, T will not increase. So, brightness will
        // not decrease. If fans are not good enough, T would increase more and more. But with this approach as
        // soon as T reaches 80 degrees, LEDs are off. Imagine, that fans are bad and T increases from 70 to 75. In
        // this case LEDs will work on 50% only, which lead to decreasing of their temperature closer to 70. I guess
        // system will find ballance, let's say on 73. At that temperature LEDs will work on ex. 70% of maximum
        // power and fans will be able to remove amount of head preventing LEDs from heating up higher.
    }
    else {
        if (is_max_fan_speed_enabled_) {
            is_max_fan_speed_enabled_ = false;
            led_driver_.SetThermalFactor(1.0);  // Restore original thermal factor
            AdjustFanSpeed(temperature);        // Adjust fan speed according to temperature
        }
    }
}
