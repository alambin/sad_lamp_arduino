#ifndef THERMALCONTROLLERMOCKS_H_
#define THERMALCONTROLLERMOCKS_H_

#include <math.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

#define PROGMEM
#define F(x)    (x)
#define PSTR(x) (x)

struct String
{
    String()
    {
    }
    explicit String(const char* ch)
      : str{ch}
    {
    }
    String(float f, int d)
      : str{std::to_string(f)}
    {
    }
    explicit String(int32_t i)
      : str{std::to_string(i)}
    {
    }

    std::string str;
};
static std::string
operator+(const char* ch, String const& str)
{
    return std::string(ch) + str.str;
}
static std::string
operator+(std::string const& l, String const& r)
{
    return l + r.str;
}
static std::string
operator+(String const& l, String const& r)
{
	return l.str + r.str;
}

static void
DebugPrintln(std::string const& str)
{
    std::cout << str << std::endl;
}

class ThermoSensors
{
public:
    ThermoSensors() = default;
    void
    GetTemperatures(float (&temperatures)[2]) const
    {
        temperatures[0] = t_;
        temperatures[0] = t_;
    }
    void
    SetTemperature(float T)
    {
        t_ = T;
    }
    void
    Loop()
    {
    }
    constexpr static float kInvalidTemperature = -127;

private:
    float t_{20.0};
};

class FanPWM
{
public:
    FanPWM() = default;
    void
    SetSpeed(uint8_t speed)
    {
        speed_ = speed;
        std::cout << "FAN speed changed to " << std::to_string(((float)speed / 255.0) * 100) << std::endl;
    }
    uint8_t
    GetSpeed() const
    {
        return speed_;
    }

private:
    uint8_t speed_{0};
};

class LedDriver
{
public:
    LedDriver() = default;
    void
    SetThermalFactor(float k)
    {
        thermal_factor_ = k;
    }
    float
    SetThermalFactor() const
    {
        return thermal_factor_;
    }

private:
    float thermal_factor_{1.0};
};

struct SerialType
{
    static void
    println(std::string const& str)
    {
        std::cout << str << std::endl;
    }
};
extern SerialType Serial;

static float
max(float l, float r)
{
    return std::max(l, r);
}

static uint32_t
millis()
{
    return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() &
            0xFFFFFFFF);
}

#endif  // THERMALCONTROLLERMOCKS_H_
