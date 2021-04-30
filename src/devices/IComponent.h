#ifndef ICOMPONENT_H_
#define ICOMPONENT_H_

class IComponent
{
public:
    virtual ~IComponent() = default;

    virtual void Setup() = 0;
};

#endif  // ICOMPONENT_H_
