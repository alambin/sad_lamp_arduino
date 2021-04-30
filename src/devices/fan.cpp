#include "fan.h"

// TODO: remove it when debugging is finished
#include <Arduino.h>

FanPWM::FanPWM(uint8_t pin, Pwm::PWMSpeed pwm_speed)
  : pwm_{pin, pwm_speed, false}
{
}

void
FanPWM::Setup()
{
    pwm_.Setup();
}

void
FanPWM::SetSpeed(uint8_t current_speed)
{
    pwm_.SetDuty(current_speed);
// Serial.println(String(F("LAMBIN FanPWM::SetSpeed(): ")) + String((((float)current_speed / 255.0) * 100), 2));
Serial.println(String(F("LAMBIN FanPWM::SetSpeed(): ")) + String(current_speed));
}
