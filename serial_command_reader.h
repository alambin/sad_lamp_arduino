#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include "IComponent.h"

#include <WString.h>

class SerialCommandReader : public IComponent
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

    SerialCommandReader();
    void    setup() override;
    bool    is_command_ready() const;
    Command get_command();

    void on_serial_event();

private:
    String input_data_;
    bool   is_input_data_ready_;
};

#endif  // SERIAL_PORT_H_
