#include "lamp_controller.h"

#include <Arduino.h>

#include "serial_command_reader.h"

namespace
{
const uint8_t  kled_driver_pin        = 9;
const uint8_t  kpotentiometer_pin     = A0;
const uint8_t  kfan1_pin              = 2;
const uint8_t  kfan2_pin              = 4;
const uint16_t kmanual_mode_threshold = 100;

// const uint16_t knum_of_pwm_steps      = 10;
// const uint16_t kpwm_frequency         = 3;
}  // namespace

LampController::LampController()
  : led_driver_(kled_driver_pin, Pwm::PWMSpeed::HZ_490)
  , potentiometer_(kpotentiometer_pin, 10)
// , fan_(3, Pwm::PWMSpeed::HZ_31372)
// , dout_pwm_(kfan1_pin, kfan2_pin)
{
}

void
LampController::setup()
{
    print_usage();
    Serial.println(F("Initializing..."));

    timer_.setup();
    timer_.register_alarm_handler(this);
    led_driver_.setup();
    potentiometer_.setup();

    Serial.println(F("Done"));

    // Temp solution - use DOUT PWM. It was used before I could run PWM module on proper PWM speed.
    // With proper HW you can use regular PWM, so, this object is not required and can be replaced.
    // dout_pwm_.setup();
    // dout_pwm_.set_output(false);
    // dout_pwm_.set_pwm_frequency(kpwm_frequency);
    // dout_pwm_.set_pwm_steps_number(knum_of_pwm_steps);
}

void
LampController::loop()
{
    // TODO:
    // 1. implement manual mode for lamp. In this mode timer should not trigger alarm.
    //    Looks like we can skip calling timer.loop() and led_driver.loop() in manual mode - need to check carefully
    //    We should properly handle entrance to manual mode. Ex. stop sunrise, if it was in progress, etc.
    // 2. Need to extend led_driver interface to support setting of brightness
    // 3. Implement inside led_driver mapping from potentiometer value to driver's duty cycle. Somthing like
    //    uint16_t rotat     = potentiometer_val / 4;
    //    uint16_t inv_rotat = 255 - rotat;
    // 4. Set manual brightness on lamp only if it differs from previously set value on signigicant amount (5-10 ? )
    //
    potentiometer_.loop();
    auto potentiometer_value = potentiometer_.read();
    if (potentiometer_value >= kmanual_mode_threshold) {
    }

    timer_.loop();
    led_driver_.loop();

    if (SerialCommandReader::instance().is_command_ready()) {
        auto command{SerialCommandReader::instance().get_command()};
        switch (command.type) {
        case SerialCommandReader::Command::CommandType::SET_TIME:
            timer_.set_time_str(command.arguments);
            break;
        case SerialCommandReader::Command::CommandType::SET_ALARM:
            timer_.set_alarm_str(command.arguments);
            break;
        case SerialCommandReader::Command::CommandType::TOGGLE_ALARM:
            timer_.toggle_alarm();
            break;
        case SerialCommandReader::Command::CommandType::SET_SUNRISE_DURATION:
            led_driver_.set_sunrise_duration_str(command.arguments);
            break;
        case SerialCommandReader::Command::CommandType::SET_FAN_PWM_FREQUENCY:
            // dout_pwm_.set_pwm_frequency(command.arguments);
            break;
        case SerialCommandReader::Command::CommandType::SET_FAN_PWM_STEPS_NUMBER:
            // dout_pwm_.set_pwm_steps_number(command.arguments);
            break;
        default:
            Serial.print(F("Unknown command: "));
            Serial.println(command.arguments);
            break;
        }
    }

    // TODO: remove it. This is temporary code to show device is alive
    static uint32_t last_printed_message_time = 0;
    auto            now                       = millis();
    if ((now - last_printed_message_time) >= 1000) {
        last_printed_message_time = now;
        auto time{timer_.get_time_str()};
        if (time.length() == 0) {
            Serial.println(F("error"));
            delay(5000);
            return;
        }

        Serial.print("Potentiometer = ");
        Serial.println(potentiometer_.read());
        Serial.println(time);
    }
}

void
LampController::on_alarm()
{
    Serial.println(F("ALARM !!!"));
    led_driver_.start_sunrise();
}

void
LampController::print_usage() const
{
    Serial.println(F("SAD lamp controller.\n"
                     "Available commands:\n"
                     "\t\"st HH:MM:SS DD/MM/YYYY\" - set current time\n"
                     "\t\"sa HH:MM\" - set alarm on specified time\n"
                     "\t\"ta\" toggle alarm On/Off\n"
                     "\t\"ssd MM\" set Sunrise duration in minutes\n"
                     "\t\"sff FF\" set fan PWM frequency (used only for DOUT PWM)\n"
                     "\t\"sfs NN\" set fan PWM steps number (steps per PWM period) (used only for DOUT PWM)\n"));
}
