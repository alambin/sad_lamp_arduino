#include "doutpwm.h"

#include <Arduino.h>
#include "eeprom_map.h"

#include "..\utils.h"

DoutPwm::DoutPwm(uint8_t pin1, uint8_t pin2)
  : pin1_{pin1}
  , pin2_{pin2}
  , frequency_{3}
  , num_of_steps_{10}
  , on_time_us_{0}
  , off_time_us_{0}
  , current_output_signal_{false}
  , is_pwm_started_{false}
  , pwm_start_time_{0}
{
}

void
DoutPwm::setup()
{
    pinMode(pin1_, OUTPUT);
    pinMode(pin2_, OUTPUT);
    set_output(false);

    uint16_t frequency{eeprom_read_word(&fan_pwm_frequency_address)};
    set_pwm_frequency(frequency);
    uint8_t steps_number{eeprom_read_byte(&fan_pwm_steps_number_address)};
    set_pwm_steps_number(steps_number);

    Serial.print(F("Read from EEPROM: fan PWM frequency ("));
    Serial.print(frequency);
    Serial.print(F(") and steps number ("));
    Serial.print((int)steps_number);
    Serial.println(F(")"));
}

void
DoutPwm::loop()
{
    if (!is_pwm_started_ || (on_time_us_ == 0) || (off_time_us_ == 0)) {
        return;
    }

    auto current_time = micros();
    auto delta_us     = current_time - pwm_start_time_;

    if (current_output_signal_) {
        if (delta_us >= on_time_us_) {
            current_output_signal_ = false;
            digitalWrite(pin1_, LOW);
            digitalWrite(pin2_, LOW);
        }
    }
    else {
        if (delta_us >= (on_time_us_ + off_time_us_)) {
            current_output_signal_ = true;
            digitalWrite(pin1_, HIGH);
            digitalWrite(pin2_, HIGH);
            // Adjust start time to avoid problems with overflow of "delta"
            pwm_start_time_ += on_time_us_ + off_time_us_;
        }
    }
}

void
DoutPwm::set_output(bool is_high)
{
    digitalWrite(pin1_, is_high ? HIGH : LOW);
    digitalWrite(pin2_, is_high ? HIGH : LOW);
    is_pwm_started_ = false;
}

void
DoutPwm::set_pwm_frequency(uint16_t frequency)
{
    frequency_      = frequency;
    is_pwm_started_ = false;
}

void
DoutPwm::set_pwm_frequency(const String& str)
{
    Serial.print(F("Received command 'Set fan PWM frequency' "));
    Serial.println(str);

    uint16_t frequency{(uint16_t)str.substring(0).toInt()};
    eeprom_write_word(&fan_pwm_frequency_address, frequency);
    set_pwm_frequency(frequency);

    Serial.print(F("Stored to EEPROM fan PWM frequency "));
    Serial.println(frequency);
}

void
DoutPwm::set_pwm_steps_number(uint8_t num_of_steps)
{
    num_of_steps_   = num_of_steps;
    is_pwm_started_ = false;
}

void
DoutPwm::set_pwm_steps_number(const String& str)
{
    Serial.print(F("Received command 'Set fan PWM steps number' "));
    Serial.println(str);

    uint8_t steps_number{(uint8_t)str.substring(0).toInt()};
    eeprom_write_byte(&fan_pwm_steps_number_address, steps_number);
    set_pwm_steps_number(steps_number);

    Serial.print(F("Stored to EEPROM fan PWM steps number "));
    Serial.println((int)steps_number);
}

void
DoutPwm::set_duty(uint8_t duty)
{
    if (duty > num_of_steps_) {
        DebugPrint(F("ERROR: duty value is higher than num_of_steps_. duty = "));
        DebugPrint(duty);
        DebugPrint(F("; num_of_steps_ = "));
        DebugPrintln(num_of_steps_);
        return;
    }

    const uint32_t step_duration_us_{1000000 / (frequency_ * num_of_steps_)};
    on_time_us_  = duty * step_duration_us_;
    off_time_us_ = (num_of_steps_ - duty) * step_duration_us_;

    DebugPrint(F("DoutPwm::set_duty(): on_time_us_ = "));
    DebugPrint(on_time_us_);
    DebugPrint(F("; off_time_us_ = "));
    DebugPrintln(off_time_us_);

    is_pwm_started_ = true;
    pwm_start_time_ = micros();

    current_output_signal_ = (on_time_us_ > 0);
    digitalWrite(pin1_, current_output_signal_ ? HIGH : LOW);
    digitalWrite(pin2_, current_output_signal_ ? HIGH : LOW);
}
