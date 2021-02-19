#include "lamp_controller.h"

#include <Arduino.h>

#include "serial_command_reader.h"

namespace
{
const uint8_t kled_driver_pin    = 9;
const uint8_t kpotentiometer_pin = A0;
const uint8_t kfan1_pin          = 2;
const uint8_t kfan2_pin          = 4;

const uint16_t kmanual_mode_level      = 100;
const uint16_t kmanual_mode_hysteresis = 0.1 * kmanual_mode_level;
const uint16_t kmanual_mode_threshold  = 4;

// const uint16_t knum_of_pwm_steps      = 10;
// const uint16_t kpwm_frequency         = 3;
}  // namespace

LampController::LampController()
  : led_driver_(kled_driver_pin, Pwm::PWMSpeed::HZ_490)
  , potentiometer_(kpotentiometer_pin, 10)
  // , fan_(3, Pwm::PWMSpeed::HZ_31372)
  // , dout_pwm_(kfan1_pin, kfan2_pin)
  , is_manual_mode_{false}
  , last_potentiometer_val_{0XFFFF}
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
    // TODO: BUG. By some reason data read from EEPROM with errors. Ex. toggle-alarm frag or alarm time.
    // RTC data seems not corrupted.

    // TODO: why fans stopped making noice? May be because of that (wrong settings?) LED driver is reeeeally slowly
    // reacts on changing of PWM? Input LED of key-module reacts immediately. Is driver reacts fast when connected
    // directly to potentiometer?

    potentiometer_.loop();
    handle_manual_mode();

    if (is_manual_mode_) {
        // Set brightness manually if current potentiometer value differs from previous
        auto potentiometer_val = potentiometer_.read();
        if (abs(potentiometer_val - last_potentiometer_val_) >= kmanual_mode_threshold) {
            last_potentiometer_val_ = potentiometer_val;
            led_driver_.set_brightness(potentiometer_.read());
        }

        // In manual mode we are not reacting on alarm from timer and not running sunrise.
    }
    else {
        timer_.check_alarm();
        led_driver_.run_sunrise();
    }

    process_commands_from_serial();

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

        // Just for debugging
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
LampController::process_commands_from_serial()
{
    if (SerialCommandReader::instance().is_command_ready()) {
        auto command{SerialCommandReader::instance().read_command()};
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
}

void
LampController::handle_manual_mode()
{
    auto potentiometer_value = potentiometer_.read();
    if (!is_manual_mode_) {
        if (potentiometer_value >= kmanual_mode_level) {
            enable_manual_mode();
        }
    }
    else if (potentiometer_value < (kmanual_mode_level - kmanual_mode_hysteresis)) {
        disable_manual_mode();
    }
}

void
LampController::enable_manual_mode()
{
    is_manual_mode_ = true;
    led_driver_.stop_sunrise();  // Stop sunrise. Just in case it was in progress
    Serial.println(F("Manual mode enabled"));
}

void
LampController::disable_manual_mode()
{
    is_manual_mode_ = false;
    Serial.println(F("Manual mode disabled"));
}

void
LampController::print_usage() const
{
    Serial.println(F("SAD lamp controller.\n"
                     "Available commands:\n"
                     "\t\"st HH:MM:SS DD/MM/YYYY\" - set current time\n"
                     "\t\"sa HH:MM WW\" - set alarm on specified time (WW - day of week mask)\n"
                     "\t\"ta\" toggle alarm On/Off\n"
                     "\t\"ssd MM\" set Sunrise duration in minutes\n"
                     "\t\"sff FF\" set fan PWM frequency (used only for DOUT PWM)\n"
                     "\t\"sfs NN\" set fan PWM steps number (steps per PWM period) (used only for DOUT PWM)\n"));
}
