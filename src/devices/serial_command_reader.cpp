#include "serial_command_reader.h"

#include <Arduino.h>

namespace
{
const char         esp_prefix[] PROGMEM = "ESP:";
const char         reset_esp[] PROGMEM  = "RESETESP";
constexpr uint32_t serial_speed         = 9600L;
}  // namespace

void
SerialCommandReader::Setup()
{
    Serial.begin(serial_speed);
}

void
SerialCommandReader::Loop()
{
    HandleSerialInactivity();

    while (Serial.available() > 0) {
        last_received_symbol_time_ = millis();
        char ch                    = Serial.read();
        if (ch == '\r') {
            // Ignore this line ending. If ESP doesn't use println() for communication with Arduino,
            // it should never happen.
            continue;
        }
        if (ch != '\n') {
            buffer_[current_buf_position_++] = ch;
            continue;
        }

        buffer_[current_buf_position_] = 0;
        auto command_length            = current_buf_position_;
        current_buf_position_          = 0;

        if ((command_length < 4) || strncmp_P(buffer_, esp_prefix, 4)) {
            // Ignore all short lines and lines without prefix
            continue;
        }

        input_data_ = &buffer_[5];
    }
}

bool
SerialCommandReader::IsCommandReady() const
{
    return (input_data_.length() != 0);
}

SerialCommandReader::Command
SerialCommandReader::ReadCommand()
{
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
    else if (command_str == "gt") {
        type = Command::CommandType::GET_TIME;
    }
    else if (command_str == "sa") {
        type = Command::CommandType::SET_ALARM;
    }
    else if (command_str == "ga") {
        type = Command::CommandType::GET_ALARM;
    }
    else if (command_str == "ea") {
        type = Command::CommandType::ENABLE_ALARM;
    }
    else if (command_str == "ta") {
        type = Command::CommandType::TOGGLE_ALARM;
    }
    else if (command_str == "ssd") {
        type = Command::CommandType::SET_SUNRISE_DURATION;
    }
    else if (command_str == "gsd") {
        type = Command::CommandType::GET_SUNRISE_DURATION;
    }
    else if (command_str == "sb") {
        type = Command::CommandType::SET_BRIGHTNESS;
    }
    else if (command_str == "gb") {
        type = Command::CommandType::GET_BRIGHTNESS;
    }
    else if (command_str == "sff") {
        type = Command::CommandType::SET_FAN_PWM_FREQUENCY;
    }
    else if (command_str == "sfs") {
        type = Command::CommandType::SET_FAN_PWM_STEPS_NUMBER;
    }
    else if (command_str == "connect") {
        type = Command::CommandType::CONNECT;
    }
    else {
        return {Command::CommandType::INVALID, input_string};
    }

    return {type, arguments};
}

void
SerialCommandReader::HandleSerialInactivity()
{
    constexpr uint32_t serial_inactivity_timeout = 1000;
    if ((current_buf_position_ != 0) && ((millis() - last_received_symbol_time_) >= serial_inactivity_timeout)) {
        current_buf_position_ = 0;
    }
}
