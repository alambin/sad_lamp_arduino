#include "lamp_controller.h"

#include <Arduino.h>

// This macro is defined for ESP, but not defined for Arduino. It is used to get access to strings in Flash
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper*>(pstr_pointer))

namespace
{
constexpr uint8_t kLedDriverPin{9};
constexpr uint8_t kPotentiometerPin{A0};
constexpr uint8_t kFan1Pin{3};
constexpr uint8_t kFan2Pin{4};
constexpr uint8_t kThermalSensorsPin{5};

// Manual mode is when potentiometer value is higher than 100. If its value is lower, it is treated as automatic mode.
// Such features as sunrise, manual brightness control via WebUI (ESP) are allowed only in automatic mode.
constexpr uint16_t kmanual_mode_level{100};
constexpr uint16_t kmanual_mode_hysteresis{0.1 * kmanual_mode_level};
constexpr uint16_t kmanual_mode_threshold{4};

// To call ESP reset user should change from manual to auto mode <kreset_esp_num_of_steps> times with being in each
// step from <kreset_esp_step_timeout_min> to <kreset_esp_step_timeout_max> milliseconds.
constexpr uint32_t kreset_esp_step_timeout_min{2000};
constexpr uint32_t kreset_esp_step_timeout_max{4000};
constexpr uint8_t  kreset_esp_num_of_steps{5};

// constexpr uint16_t knum_of_pwm_steps      = 10;
// constexpr uint16_t kpwm_frequency         = 3;

constexpr char esp_set_time_ack[] PROGMEM             = "TOESP: st ACK\n";
constexpr char esp_get_time_ack[] PROGMEM             = "TOESP: gt ACK ";
constexpr char esp_set_alarm_ack[] PROGMEM            = "TOESP: sa ACK\n";
constexpr char esp_get_alarm_ack[] PROGMEM            = "TOESP: ga ACK ";
constexpr char esp_enable_alarm_ack[] PROGMEM         = "TOESP: ea ACK ";
constexpr char esp_toggle_alarm_ack[] PROGMEM         = "TOESP: ta ACK\n";
constexpr char esp_set_sunrise_duration_ack[] PROGMEM = "TOESP: ssd ACK\n";
constexpr char esp_get_sunrise_duration_ack[] PROGMEM = "TOESP: gsd ACK ";
constexpr char esp_set_brightness_ack[] PROGMEM       = "TOESP: sb ACK ";
constexpr char esp_get_brightness_ack[] PROGMEM       = "TOESP: gb ACK ";
constexpr char esp_set_pwm_frequency_ack[] PROGMEM    = "TOESP: sff ACK\n";
constexpr char esp_set_pwm_steps_number_ack[] PROGMEM = "TOESP: sfs ACK\n";
constexpr char esp_connect_ack[] PROGMEM              = "TOESP: connect ACK\n";
constexpr char esp_reset_cmd[] PROGMEM                = "TOESP: RESETESP\n";
}  // namespace

LampController::LampController()
  : led_driver_(kLedDriverPin, Pwm::PWMSpeed::HZ_490)
  , potentiometer_(kPotentiometerPin, 10)
  // TODO: need to have 1 more fan. Or adapt code of fan to control 2 fans
  , fan_(kFan1Pin, Pwm::PWMSpeed::HZ_31372)
  // , dout_pwm_(kFan1Pin, kFan2Pin)
  , thermo_sensors_(kThermalSensorsPin)
  , thermal_controller_(thermo_sensors_, fan_, led_driver_)
  , is_manual_mode_{false}
  , last_potentiometer_val_{0XFFFF}
  , last_mode_switch_time_{0}
{
}

void
LampController::Setup()
{
    serial_command_reader_.Setup();

    PrintUsage();
    Serial.println(F("Initializing..."));

    timer_.Setup();
    timer_.RegisterAlarmHandler(this);
    led_driver_.Setup();
    potentiometer_.Setup();
    fan_.Setup();
    thermo_sensors_.Setup();

    Serial.println(F("Done"));

    // Temp solution - use DOUT PWM. It was used before I could run PWM module on proper PWM speed.
    // With proper HW you can use regular PWM, so, this object is not required and can be deleted.
    // dout_pwm_.setup();
    // dout_pwm_.set_output(false);
    // dout_pwm_.set_pwm_frequency(kpwm_frequency);
    // dout_pwm_.set_pwm_steps_number(knum_of_pwm_steps);
}

void
LampController::Loop()
{
    thermo_sensors_.Loop();
    thermal_controller_.Loop();

    potentiometer_.Loop();
    HandleManualMode();

    if (is_manual_mode_) {
        // Set brightness manually if current potentiometer value differs from previous
        auto potentiometer_val = potentiometer_.Read();
        if (abs(potentiometer_val - last_potentiometer_val_) >= kmanual_mode_threshold) {
            last_potentiometer_val_ = potentiometer_val;
            led_driver_.SetBrightness(potentiometer_.Read());
        }

        // In manual mode we are not reacting on alarm from timer and not running sunrise.
    }
    else {
        timer_.CheckAlarm();
        led_driver_.RunSunrise();
    }

    ProcessCommandsFromSerial();

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
LampController::OnAlarm()
{
    // TODO: remove this log in production
    Serial.println(F("ALARM !!!"));
    led_driver_.StartSunrise();
}

void
LampController::ProcessCommandsFromSerial()
{
    serial_command_reader_.Loop();
    if (serial_command_reader_.IsCommandReady()) {
        auto command{serial_command_reader_.ReadCommand()};
        switch (command.type) {
        case SerialCommandReader::Command::CommandType::SET_TIME:
            timer_.SetTimeStr(command.arguments);
            Serial.print(String(FPSTR(esp_set_time_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_TIME:
            Serial.print(String(FPSTR(esp_get_time_ack)) + timer_.GetTimeStr() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::SET_ALARM:
            timer_.SetAlarmStr(command.arguments);
            Serial.print(String(FPSTR(esp_set_alarm_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_ALARM:
            Serial.print(String(FPSTR(esp_get_alarm_ack)) + timer_.GetAlarmStr() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::ENABLE_ALARM: {
            bool result{timer_.EnableAlarmStr(command.arguments)};
            Serial.print(String(FPSTR(esp_enable_alarm_ack)) + (result ? F("DONE\n") : F("ERROR\n")));
            break;
        }
        case SerialCommandReader::Command::CommandType::TOGGLE_ALARM:
            timer_.ToggleAlarm();
            Serial.print(String(FPSTR(esp_toggle_alarm_ack)));
            break;
        case SerialCommandReader::Command::CommandType::SET_SUNRISE_DURATION:
            led_driver_.SetSunriseDurationStr(command.arguments);
            Serial.print(String(FPSTR(esp_set_sunrise_duration_ack)));
            break;
        case SerialCommandReader::Command::CommandType::GET_SUNRISE_DURATION:
            Serial.print(String(FPSTR(esp_get_sunrise_duration_ack)) + led_driver_.GetSunriseDurationStr() + "\n");
            break;
        case SerialCommandReader::Command::CommandType::SET_BRIGHTNESS:
            if (is_manual_mode_) {
                Serial.print(String(FPSTR(esp_set_brightness_ack)) + F("ERROR: manual mode\n"));
                break;
            }
            led_driver_.SetBrightnessStr(command.arguments);
            Serial.print(String(FPSTR(esp_set_brightness_ack)) + F("DONE\n"));
            break;
        case SerialCommandReader::Command::CommandType::GET_BRIGHTNESS:
            Serial.print(String(FPSTR(esp_get_brightness_ack)) + (is_manual_mode_ ? "M " : "A ") +
                         led_driver_.GetBrightnessStr() + "\n");
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
LampController::HandleManualMode()
{
    auto potentiometer_value = potentiometer_.Read();
    if (!is_manual_mode_) {
        if (potentiometer_value >= kmanual_mode_level) {
            EnableManualMode();
        }
    }
    else if (potentiometer_value < (kmanual_mode_level - kmanual_mode_hysteresis)) {
        DisableManualMode();
    }
}

void
LampController::HandleEspResetRequest()
{
    static uint8_t correct_steps_counter{0};
    auto           delta   = millis() - last_mode_switch_time_;
    last_mode_switch_time_ = millis();

    if ((kreset_esp_step_timeout_max >= delta) && (delta >= kreset_esp_step_timeout_min)) {
        if (++correct_steps_counter >= kreset_esp_num_of_steps) {
            Serial.print(FPSTR(esp_reset_cmd));
            correct_steps_counter = 0;
        }
    }
    else {
        correct_steps_counter = 0;
    }
}

void
LampController::EnableManualMode()
{
    is_manual_mode_ = true;
    led_driver_.StopSunrise();  // Stop sunrise. Just in case it was in progress
    Serial.println(F("Manual mode enabled"));

    HandleEspResetRequest();
}

void
LampController::DisableManualMode()
{
    is_manual_mode_ = false;
    Serial.println(F("Manual mode disabled"));

    HandleEspResetRequest();
}

void
LampController::PrintUsage() const
{
    Serial.println(
        F("SAD lamp controller.\n"
          "Available commands:\n"
          "\t\"ESP: st HH:MM:SS DD/MM/YYYY\" - set current time\n"
          "\t\"ESP: gt\" - get current time (HH:MM:SS DD/MM/YYYY)\n"
          "\t\"ESP: sa HH:MM WW\" - set alarm on specified time (WW - day of week mask)\n"
          "\t\"ESP: ga\" - get alarm (E HH:MM WW, E = \"E\" if alarm enabled, \"D\" if disabled)\n"
          "\t\"ESP: ea E\" enable alarm (if E = \"E\", enable alarm, if E = \"D\", disable)\n"
          "\t\"ESP: ta\" toggle alarm On/Off\n"
          "\t\"ESP: ssd MMMM\" set Sunrise duration in minutes (0-1440)\n"
          "\t\"ESP: gsd\" get Sunrise duration (MMMM)\n"
          "\t\"ESP: sb BBBB\" set brightness (0-1023). Not allowed in manual lamp control mode\n"
          "\t\"ESP: gb\" get current brightness (M BBBB, M = \"M\" if lamp in manual mode, \"A\" - in automatic mode)\n"
          "\t\"ESP: sff FF\" set fan PWM frequency (used only for DOUT PWM)\n"
          "\t\"ESP: sfs NN\" set fan PWM steps number (steps per PWM period) (used only for DOUT PWM)\n"));
}
