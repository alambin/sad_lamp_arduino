#ifndef ICOMPONENT_H_
#define ICOMPONENT_H_

class IComponent
{
public:
    ~IComponent() = default;

    virtual void setup() = 0;
};

#endif  // ICOMPONENT_H_
