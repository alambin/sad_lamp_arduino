#include "lamp_controller.h"

#include <Arduino.h>

// This macro is defined for ESP, but not defined for Arduino. It is used to get access to strings in Flash
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper*>(pstr_pointer))

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

constexpr char esp_set_time_ack[] PROGMEM             = "TOESP: st ACK\n";
constexpr char esp_get_time_ack[] PROGMEM             = "TOESP: gt ACK ";
constexpr char esp_set_alarm_ack[] PROGMEM            = "TOESP: sa ACK\n";
constexpr char esp_get_alarm_ack[] PROGMEM            = "TOESP: ga ACK ";
constexpr char esp_enable_alarm_ack[] PROGMEM         = "TOESP: ea ACK ";
constexpr char esp_toggle_alarm_ack[] PROGMEM         = "TOESP: ta ACK\n";
constexpr char esp_set_sunrise_duration_ack[] PROGMEM = "TOESP: ssd ACK\n";
constexpr char esp_get_sunrise_duration_ack[] PROGMEM = "TOESP: gsd ACK ";
constexpr char esp_set_pwm_frequency_ack[] PROGMEM    = "TOESP: sff ACK\n";
constexpr char esp_set_pwm_steps_number_ack[] PROGMEM = "TOESP: sfs ACK\n";
constexpr char esp_connect_ack[] PROGMEM              = "TOESP: connect ACK\n";
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
    serial_command_reader_.setup();

    print_usage();
    Serial.println(F("Initializing..."));

    timer_.setup();
    timer_.register_alarm_handler(this);
    led_driver_.setup();
    potentiometer_.setup();

    Serial.println(F("Done"));

    // Temp solution - use DOUT PWM. It was used before I could run PWM module on proper PWM speed.
    // With proper HW you can use regular PWM, so, this object is not required and can be deleted.
    // dout_pwm_.setup();
    // dout_pwm_.set_output(false);
    // dout_pwm_.set_pwm_frequency(kpwm_frequency);
    // dout_pwm_.set_pwm_steps_number(knum_of_pwm_steps);
}

void
LampController::loop()
{
    // TODO: BUG. By some reason data read from EEPROM with errors. Ex. toggle-alarm frag or alarm time.
    //       RTC data seems not corrupted.
    //       Once I entered data at evening and checked next morning - everything is OK. But day before data was
    //       corrupted often during my work with Arduino. May be reason is corruption due to mess with addresses? Ex.
    //       overwriting of one data by another.

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
    // static uint32_t last_printed_message_time = 0;
    // auto            now                       = millis();
    // if ((now - last_printed_message_time) >= 1000) {
    //     last_printed_message_time = now;
    //     auto time{timer_.get_time_str()};
    //     if (time.length() == 0) {
    //         Serial.println(F("error"));
    //         delay(5000);
    //         return;
    //     }

    //     // Just for debugging
    //     Serial.print("Potentiometer = ");
    //     Serial.println(potentiometer_.read());
    //     Serial.println(time);
    // }
}

void
LampController::on_alarm()
{
    // TODO: remove this log in production
    Serial.println(F("ALARM !!!"));
    led_driver_.start_sunrise();
}

void
LampController::process_commands_from_serial()
{
    serial_command_reader_.loop();
    if (serial_command_reader_.is_command_ready()) {
        auto command{serial_command_reader_.read_command()};
        switch (command.type) {
        case SerialCommandReader::Command::CommandType::SET_TIME:
            timer_.set_time_str(command.arguments);
            Serial.print(String(FPSTR(esp_set_time_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_TIME:
            Serial.print(String(FPSTR(esp_get_time_ack)) + timer_.get_time_str() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::SET_ALARM:
            timer_.set_alarm_str(command.arguments);
            Serial.print(String(FPSTR(esp_set_alarm_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_ALARM:
            Serial.print(String(FPSTR(esp_get_alarm_ack)) + timer_.get_alarm_str() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::ENABLE_ALARM: {
            bool result{timer_.enable_alarm_str(command.arguments)};
            Serial.print(String(FPSTR(esp_enable_alarm_ack)) + (result ? F("DONE\n") : F("ERROR\n")));
            break;
        }
        case SerialCommandReader::Command::CommandType::TOGGLE_ALARM:
            timer_.toggle_alarm();
            Serial.print(String(FPSTR(esp_toggle_alarm_ack)));
            break;
        case SerialCommandReader::Command::CommandType::SET_SUNRISE_DURATION:
            led_driver_.set_sunrise_duration_str(command.arguments);
            Serial.print(String(FPSTR(esp_set_sunrise_duration_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_SUNRISE_DURATION:
            Serial.print(String(FPSTR(esp_get_sunrise_duration_ack)) + led_driver_.get_sunrise_duration_str() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::SET_FAN_PWM_FREQUENCY:
            // dout_pwm_.set_pwm_frequency(command.arguments);
            Serial.print(String(FPSTR(esp_set_pwm_frequency_ack)));
            break;
        case SerialCommandReader::Command::CommandType::SET_FAN_PWM_STEPS_NUMBER:
            // dout_pwm_.set_pwm_steps_number(command.arguments);
            Serial.print(String(FPSTR(esp_set_pwm_steps_number_ack)));
            break;
        case SerialCommandReader::Command::CommandType::CONNECT:
            Serial.print(FPSTR(esp_connect_ack));
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

// TODO: add sending ACKs for all commands!
// TODO: add command to get current brightness in format "M BBBB", where M = 'M' if manual mode, or 'A' if automated
// mode
// TODO: add command to set brightness if mode is automated ????
// TODO: should Arduino notify ESP when entering Auto mode (so it will be able to set brightness from WebUI)?
//       Isn't it overkill?

void
LampController::print_usage() const
{
    Serial.println(F("SAD lamp controller.\n"
                     "Available commands:\n"
                     "\t\"ESP: st HH:MM:SS DD/MM/YYYY\" - set current time\n"
                     "\t\"ESP: gt\" - get current time (HH:MM:SS DD/MM/YYYY)\n"
                     "\t\"ESP: sa HH:MM WW\" - set alarm on specified time (WW - day of week mask)\n"
                     "\t\"ESP: ga\" - get alarm (E HH:MM WW, E = \"E\" if alarm enabled, \"D\" if disabled)\n"
                     "\t\"ESP: ea E\" enable alarm (if E = \"E\", enable alarm, if E = \"D\", disable)\n"
                     "\t\"ESP: ta\" toggle alarm On/Off\n"
                     "\t\"ESP: ssd MMMM\" set Sunrise duration in minutes (0-1440)\n"
                     "\t\"ESP: gsd\" get Sunrise duration (MMMM)\n"
                     "\t\"ESP: sff FF\" set fan PWM frequency (used only for DOUT PWM)\n"
                     "\t\"ESP: sfs NN\" set fan PWM steps number (steps per PWM period) (used only for DOUT PWM)\n"));
}
