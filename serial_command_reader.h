#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include <WString.h>

class SerialCommandReader
{
public:
    struct Command
    {
        enum class CommandType : uint8_t
        {
            SET_TIME = 0,
            SET_ALARM,
            TOGGLE_ALARM,
            SET_SUNRISE_DURATION,
            SET_FAN_PWM_FREQUENCY,
            SET_FAN_PWM_STEPS_NUMBER,
            INVALID = 255
        } type;
        String arguments;
    };

    // Singleton
    static SerialCommandReader& instance();
    SerialCommandReader(const SerialCommandReader&) = delete;
    SerialCommandReader(SerialCommandReader&&)      = delete;
    SerialCommandReader& operator=(const SerialCommandReader&) = delete;
    SerialCommandReader& operator=(const SerialCommandReader&&) = delete;

    bool    is_command_ready() const;
    Command get_command();

    void on_serial_event();

private:
    SerialCommandReader();

    String input_data_;
    bool   is_input_data_ready_;
};

#endif  // SERIAL_PORT_H_
