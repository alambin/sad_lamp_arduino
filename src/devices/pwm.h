#ifndef PWM_H_
#define PWM_H_

#include "IComponent.h"

#include <stdint.h>

class Pwm : public IComponent
{
public:
    enum class PWMSpeed : uint8_t
    {
        HZ_30 = 0,
        HZ_122,
        HZ_245,
        HZ_490,
        HZ_980,
        HZ_3921,
        HZ_31372
    };

    /*!
     * \brief Generate PWM on given pin with given speed
     * \param [in] pin Pit to generate PWM. Possible values are 5 or 6 (Timer0 - NOT RECOMMENDED), 9 or 10 (Timer1),
     * 3 or 11 (Timer2)
     * \param [in] pwm_speed Base speed of PWM
     * \param [in] double_pwm Should we double PWM speed (use fast pwm) or not (use phase correct pwm)
     */
    Pwm(uint8_t pin, PWMSpeed pwm_speed, bool double_pwm);
    void Setup() override;
    void SetDuty(uint8_t duty);

private:
    struct GetSpeedMaskResult
    {
        uint8_t mask;
        uint8_t timer_number;
    };

    GetSpeedMaskResult GetSpeedMask() const;

    const uint8_t  pin_;
    const PWMSpeed pwm_speed_;
    bool           double_pwm_;
};

#endif  // PWM_H_
