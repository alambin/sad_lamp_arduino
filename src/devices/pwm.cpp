#include "pwm.h"

#include <Arduino.h>

namespace
{
constexpr PROGMEM uint8_t mask_map[3][7] = {
    // 30 Hz      122 Hz     245 Hz     490 Hz    980 Hz     3921 Hz    31372 Hz
    {B00000101, B00000100, B00000000, B00000011, B00000000, B00000010, B00000001},   // D5, D6
    {B00000101, B00000100, B00000000, B00000011, B00000000, B00000010, B00000001},   // D9, D10
    {B00000111, B00000110, B00000101, B00000100, B00000011, B00000010, B00000001}};  // D3, D11
}  // namespace

Pwm::Pwm(uint8_t pin, Pwm::PWMSpeed pwm_speed, bool double_pwm)
  : pin_{pin}
  , pwm_speed_{pwm_speed}
  , double_pwm_{double_pwm}
{
}

void
Pwm::setup()
{
    auto res = get_speed_mask();
    if (res.mask == 0) {
        Serial.print(F("ERROR: not supported pin for PWM: "));
        Serial.println(pin_);
        return;
    }

    switch (res.timer_number) {
    case 0:
        if (double_pwm_) {
            TCCR0A |= B00000010;
        }
        TCCR0B &= B11111000;
        TCCR0B |= res.mask;
        break;
    case 1:
        TCCR1A |= 1;
        if (double_pwm_) {
            TCCR1B |= B00001000;
        }
        TCCR1B &= B11111000;
        TCCR1B |= res.mask;
        break;
    case 2:
        if (double_pwm_) {
            TCCR2A |= B00000010;
        }
        TCCR2B &= B11111000;
        TCCR2B |= res.mask;
        break;
    }
}

void
Pwm::set_duty(uint8_t duty)
{
    analogWrite(pin_, duty);
}

Pwm::GetSpeedMaskResult
Pwm::get_speed_mask() const
{
    uint8_t timer_number{0};
    switch (pin_) {
    case 5:
    case 6:
        timer_number = 0;
        break;
    case 9:
    case 10:
        timer_number = 1;
        break;
    case 3:
    case 11:
        timer_number = 2;
        break;
    default:
        return GetSpeedMaskResult{0, 0};
    }

    auto mask = pgm_read_byte(&mask_map[timer_number][static_cast<uint8_t>(pwm_speed_)]);
    if (mask == 0) {
        Serial.print(F("WARN: not supported speed "));
        Serial.print((int)pwm_speed_);
        Serial.print(F(" for PWM: "));
        Serial.print(pin_);
        Serial.println(F(". Use default speed"));
        mask = mask_map[timer_number][3];
    }
    return GetSpeedMaskResult{mask, timer_number};
}
