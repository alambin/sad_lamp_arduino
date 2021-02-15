#ifndef LED_H_
#define LED_H_

#include "IComponent.h"

class Led : public IComponent
{
public:
    Led(int pin);
    void setup() override;
    void turn_on(bool is_on);

private:
    int pin_;
};

#endif  // LED_H_
