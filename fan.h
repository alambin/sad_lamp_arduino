/*
NOTES:
1. Seems like 12V fans will only start working after 6-7V. They need quite much power to start rotating.
   But when they started, they don't need much energy to keep rotating and you can reduce voltage (ex. to 4V)
2. https://chipenable.ru/index.php/how-connection/220-kak-upravlyat-ventilyatorom.html
   - "To fight acoustic noise usually PWM frequency is increased to 20 kHz"
   - www. compress.ru/article.aspx?id=15092 Typical is 21-15 kHz
   - https://electronics.stackexchange.com/questions/11079/how-do-i-eliminate-pwm-noise-when-driving-a-fan
     "Min 25 kHz + 100-1000 uF capacitor"
   25 kHz seems reasonable as this is deffinitely out of audible frequency range
3. Even small key on 4 kHz with duty cycle 50% gives 12V. May be reason is again in too big values of input
   resistors?
   But 10k and 1k seems quite popular values. Need to research more


Temporary conclusions:
1. Input resistors of MOSFET should be taken into account. Value of pull-up resistor 10k seems to be reasonable.
   My small key doesn't work on more than 4 kHz not because of MOSFET, but because of optocoupler.
   So, need to try small key with 10k and with faster optocoupler. If it will not work - need to try less Ohm
2. This input resistor value is also determined by optocoupler. In its value determines speed of optocoupler.


Additional notes
1. If you have desired PWM frequency (Fpwm >25 kHz) and precision of your PWM (N = 2^8 = 256, but D3 of Arduino
   can give even 2^16), then based on formula Fpwm = 2/[N * (tr +tf)] you can calculate
   how fast optocoupler you need (tr +tf). Not sure if this is related to speed of MOSFET
   So, for 32 kHz, 8 bit PWM (N=256, eventhough D3 on Arduino can give 2^16) you need optocoupler with t < 244 ns
   6N137 - 86 Rub (https://www.chipdip.ru/product0/8781038226), but package is not convenient.
2. THEORY:
   Formula above guarantee that even minimum duty cycle (1/256 of period of PWM) will be passed by optocoupler.
   But if you send wider signals (several steps - more than 1/256), then this requirement is softer:
   minimum duty cycle * PWM period >= tr +tf
   Ex. if you are OK that your optocouple will pass on 30 kHz 30% duty cycle and more, then you should choose
   optocoupler with t=(tr +tf) < 0.3 * 33 uS= 10 uS
   PRACTICE:
   You should take into account that after duty cycle is more than 70%, then that optocoupler will also not pass
   it well (signal will not have enough time to fall and next period of PWM will come)
3. More frequency - more heating and more power loss. Because more time MOSFET will be in transitional state and
   consume current.
4. MOSFET in this video https://youtu.be/cU9oF3OKIlY?t=76 could work on 60+ kHz with 8 bit PWM
   IRL3705N - https://static.chipdip.ru/lib/144/DOC000144495.pdf
   It has 3 times higher input capacity (Ciss): 3600 vs 1200 (IRF5305S)
   It means that IRF5305S SHOULD WORK on 60+ kHz without driver !!!!!! My problem is most probably in optocoupler !!!

Test noise on PWM with different frequencies and with different duty cycles:
Hz    | 25%  | 50%  | 75%   | Notes
30    | 4.7V | 7.3V | 9.8V  | Small switch. Very low, but noticeable NOISE
30    | 6.8V | 9.2V | 11.5V | 4-way switch. no NOISE!
122   | 5.4V | 7.8V | 10.2V | Low NOISE
490   | 5.3V | 8.4V | 10.3V | NOISE
980   | 5.1V | 7.8V | 9.8V  | NOISE
3921  | 6.1V | 12V  | 12V   | NOISE
31372 | 12V  | 12V  | 12V   | No NOISE
*/

#ifndef FAN_H_
#define FAN_H_

#include "IComponent.h"
#include "pwm.h"

class FanPWM : public IComponent
{
public:
    FanPWM(int pin, Pwm::PWMSpeed pwm_speed);
    void setup() override;
    void set_speed(char current_speed);

private:
    Pwm pwm_;
};

#endif  // FAN_H_
