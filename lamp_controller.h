#ifndef LAMP_CONTROLLER_H_
#define LAMP_CONTROLLER_H_

#include "IComponent.h"

#include <stdint.h>

#include "fan.h"
#include "led_driver.h"
#include "potentiometer.h"
#include "timer.h"

class LampController
  : public IComponent
  , public Timer::AlarmHandler
{
public:
    LampController();
    void setup() override;
    void loop();
    void on_alarm() override;

private:
    void print_usage() const;

    Timer         timer_;
    LedDriver     led_driver_;
    Potentiometer potentiometer_;
    // FanPWM              fan_;
    // DoutPwm             dout_pwm_;
};

#endif  // LAMP_CONTROLLER_H_
