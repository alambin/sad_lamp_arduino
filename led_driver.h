#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_

#include "IComponent.h"

#include <WString.h>
#include <stdint.h>

#include "pwm.h"

// Controlls current driver for powerful LED
class LedDriver : public IComponent
{
public:
    LedDriver(uint8_t pin, Pwm::PWMSpeed pwm_speed, uint32_t updating_period_ms = 1000);
    void setup() override;
    void loop();

    void set_sunrise_duration_str(const String& str);

    void start_sunrise();

private:
    void set_sunrise_duration(uint32_t duration_m);

    Pwm            pwm_;
    const uint32_t initial_updating_period_ms_;
    uint32_t       adjusted_updating_period_ms_;
    uint32_t       last_updating_time_;
    bool           is_sunrise_in_progress_;
    uint32_t       sunrise_start_time_;
    uint32_t       sunrise_duration_sec_;
};

#endif  // LED_DRIVER_H_
