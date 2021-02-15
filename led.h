#ifndef LED_H_
#define LED_H_

#include "IComponent.h"

#include <stdint.h>

class Led : public IComponent
{
public:
    Led(uint8_t pin);
    void setup() override;
    void turn_on(bool is_on);

private:
    const uint8_t pin_;
};

#endif  // LED_H_
