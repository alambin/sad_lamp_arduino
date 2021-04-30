#ifndef DOUTPWM_H_
#define DOUTPWM_H_

#include "IComponent.h"

#include <WString.h>
#include <stdint.h>

// Implements PWM using digital output pin. The same PWM is generated on both - pin1 and pin2
// PWM is generated for frequency "frequency". Each period of PWM is split in "num_of_steps" steps.
// Each step value of output is not changed - it is either 0, either 1. So, "num_of_steps" determines PWM resolution
// Ex. if frequency is 3 Hz, num_of_steps is 10, then the shortest possible with of signal is ([1/3] / 10) = 1/30 s.
// Pay attentions that hardware should be selected to support operation on max frequency
// ("frequency" * "num_of_steps") Hz
class DoutPwm : public IComponent
{
public:
    DoutPwm(uint8_t pin1, uint8_t pin2);
    void Setup() override;
    void Loop();

    // Configure PWM. New parameters will be applied only after next PWM start (SetDuty())
    void SetPwmFrequency(uint16_t frequency);
    void SetPwmFrequency(const String& str);
    void SetPwmStepsNumber(uint8_t num_of_steps);
    void SetPwmStepsNumber(const String& str);

    // Starts PWM.
    // Should be called AFTER PWM is configured (SetPwmFrequency() and SetPwmStepsNumber() are called)
    // duty is in range [0, num_of_steps)
    void SetDuty(uint8_t duty);

    // Stops PWM and set output in specified value.
    void SetOutput(bool is_high);

private:
    const uint8_t pin1_;
    const uint8_t pin2_;
    uint16_t      frequency_;
    uint8_t       num_of_steps_;
    uint32_t      on_time_us_;
    uint32_t      off_time_us_;
    bool          current_output_signal_;

    bool     is_pwm_started_;
    uint32_t pwm_start_time_;
};

#endif  // DOUTPWM_H_
