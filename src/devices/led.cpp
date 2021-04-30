#include "led.h"

#include <Arduino.h>

Led::Led(uint8_t pin)
  : pin_{pin}
{
}

void
Led::Setup()
{
    pinMode(pin_, OUTPUT);
}

void
Led::TurnOn(bool is_on)
{
    digitalWrite(pin_, is_on ? HIGH : LOW);
}
