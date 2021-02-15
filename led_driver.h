#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_

#include "IComponent.h"

#include <stdint.h>

#include "pwm.h"
#include "timer.h"

// Controlls current driver for powerful LED
class LedDriver : public IComponent
{
public:
    LedDriver(uint8_t pin, Pwm::PWMSpeed pwm_speed, Timer& timer);
    void setup() override;
    void loop();

    void set_sunrise_duration_str(const String& str);

    void start_sunrise();

private:
    Pwm      pwm_;
    Timer&   timer_;
    bool     is_sunrise_in_progress_;
    time_t   sunrise_start_time_;
    uint16_t sunrise_duration_sec_;
};

#endif  // LED_DRIVER_H_
