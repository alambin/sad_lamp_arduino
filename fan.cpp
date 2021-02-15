#include "fan.h"

FanPWM::FanPWM(int pin, Pwm::PWMSpeed pwm_speed)
  : pwm_(pin, pwm_speed, false)
{
}

void
FanPWM::setup()
{
    pwm_.setup();
}

void
FanPWM::set_speed(char current_speed)
{
    pwm_.set_duty(current_speed);
}
