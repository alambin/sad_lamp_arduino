#include "../../src/devices/thermalcontroller.hpp"
#include "thermalcontrollermocks.h"

int
main()
{
    ThermoSensors     sensors;
    FanPWM            fan;
    LedDriver         led_driver;
    ThermalController thermal_controller(sensors, fan, led_driver);

    while (true) {
        float temperatures[2];
        sensors.GetTemperatures(temperatures);
        std::cout << "T = " << temperatures[0]
                  << "; FanSpeed = " << std::to_string(((float)fan.GetSpeed() / 255.0) * 100)
                  << "; ThermalFactor = " << std::to_string(led_driver.SetThermalFactor()) << std::endl;
        std::cout << "Enter T (0 - exit): ";

        float t;
        std::cin >> t;
		std::cout << std::endl;

        if (std::fabs(t - 0) < 0.0001) {
            return 0;
        }
        sensors.SetTemperature(t);
        thermal_controller.Loop();
    }

    return 0;
}
