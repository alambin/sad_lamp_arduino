#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_

#include "IComponent.h"

#include <WString.h>
#include <stdint.h>

#include "pwm.h"

// Controls current driver for powerful LED
class LedDriver : public IComponent
{
public:
    LedDriver(uint8_t pin, Pwm::PWMSpeed pwm_speed, uint32_t updating_period_ms = 1000);
    void Setup() override;
    void RunSunrise();

    void   SetSunriseDurationStr(const String& str);
    String GetSunriseDurationStr() const;

    void   SetBrightness(uint16_t level);  // level is in range [0..1023]
    void   SetBrightnessStr(const String& str);
    String GetBrightnessStr() const;
    void   SetThermalFactor(float thermal_factor);

    void StartSunrise();
    void StopSunrise();

private:
    void    SetSunriseDuration(uint16_t duration_m);
    uint8_t MapSunriseTimeToLevel(uint32_t delta_time_ms);
    uint8_t MapManualControlToLevel(uint16_t manual_level);

    Pwm            pwm_;
    const uint32_t initial_updating_period_ms_;
    uint32_t       adjusted_updating_period_ms_;
    bool           is_sunrise_in_progress_;
    uint32_t       sunrise_start_time_;
    uint32_t       sunrise_duration_sec_;
    uint16_t       current_brightness_;
    float          thermal_factor_;
};

#endif  // LED_DRIVER_H_
