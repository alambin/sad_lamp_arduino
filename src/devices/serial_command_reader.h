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
            GET_TIME,
            SET_ALARM,
            GET_ALARM,
            ENABLE_ALARM,
            TOGGLE_ALARM,
            SET_SUNRISE_DURATION,
            GET_SUNRISE_DURATION,
            SET_FAN_PWM_FREQUENCY,
            SET_FAN_PWM_STEPS_NUMBER,
            CONNECT,
            INVALID = 255
        } type;
        String arguments;
    };

    SerialCommandReader();
    void setup() override;
    void loop();

    bool    is_command_ready() const;
    Command read_command();

private:
    static constexpr uint8_t buffer_size_{64};
    char                     buffer_[buffer_size_];
    uint16_t                 current_buf_position_{0};

    String input_data_;
    bool   is_input_data_ready_;
};

#endif  // SERIAL_PORT_H_
