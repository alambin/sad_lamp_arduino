#ifndef LAMP_CONTROLLER_H_
#define LAMP_CONTROLLER_H_

#include "devices/IComponent.h"

#include <stdint.h>

#include "devices/fan.h"
#include "devices/led_driver.h"
#include "devices/potentiometer.h"
#include "devices/serial_command_reader.h"
#include "devices/timer.h"

class LampController
  : public IComponent
  , public Timer::AlarmHandler
{
public:
    LampController();
    void setup() override;
    void loop();
    void on_alarm() override;

private:
    void process_commands_from_serial();

    void handle_manual_mode();
    void handle_esp_reset_request();
    void enable_manual_mode();
    void disable_manual_mode();

    void print_usage() const;

    Timer               timer_;
    LedDriver           led_driver_;
    Potentiometer       potentiometer_;
    SerialCommandReader serial_command_reader_;
    // FanPWM              fan_;
    // DoutPwm             dout_pwm_;

    bool     is_manual_mode_;
    uint16_t last_potentiometer_val_;

    uint32_t last_mode_switch_time_;  // Time when last time we switched from manual to auto mode or vice versa
};

#endif  // LAMP_CONTROLLER_H_
