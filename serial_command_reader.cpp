#include "serial_command_reader.h"

#include <HardwareSerial.h>

SerialCommandReader&
SerialCommandReader::instance()
{
    static SerialCommandReader instance;
    return instance;
}

SerialCommandReader::SerialCommandReader()
  : is_input_data_ready_{false}
{
    Serial.begin(9600);
}

bool
SerialCommandReader::is_command_ready() const
{
    return is_input_data_ready_;
}

SerialCommandReader::Command
SerialCommandReader::get_command()
{
    is_input_data_ready_ = false;
    String input_string{input_data_};
    input_data_ = "";

    String command_str;
    String arguments;
    auto   space_index = input_string.indexOf(' ');
    if (space_index == -1) {
        command_str = input_string;
    }
    else {
        command_str = input_string.substring(0, space_index);
        arguments   = input_string.substring(space_index + 1);
    }

    Command::CommandType type;
    if (command_str == "st") {
        type = Command::CommandType::SET_TIME;
    }
    else if (command_str == "sa") {
        type = Command::CommandType::SET_ALARM;
    }
    else if (command_str == "ta") {
        type = Command::CommandType::TOGGLE_ALARM;
    }
    else if (command_str == "ssd") {
        type = Command::CommandType::SET_SUNRISE_DURATION;
    }
    else if (command_str == "sff") {
        type = Command::CommandType::SET_FAN_PWM_FREQUENCY;
    }
    else if (command_str == "sfs") {
        type = Command::CommandType::SET_FAN_PWM_STEPS_NUMBER;
    }
    else {
        return {Command::CommandType::INVALID, input_string};
    }

    return {type, arguments};
}

void
SerialCommandReader::on_serial_event()
{
    if (Serial.available()) {
        input_data_          = Serial.readStringUntil('\n');
        is_input_data_ready_ = true;
    }
}

// Arduino's predefined function-callback on receiving serial data
void
serialEvent()
{
    SerialCommandReader::instance().on_serial_event();
}
