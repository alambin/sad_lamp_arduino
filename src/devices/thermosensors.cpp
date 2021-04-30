#include "thermosensors.hpp"

#include <HardwareSerial.h>
#include <Streaming.h>

namespace
{
// Calibration data:
//
// 1 sensor (28B61675D0013CA2 - orange wires):
// 37.0 on medical thermometer -> 36.31 on sensor
// Boiling water (100) -> 97.25, but water in boiling pan can have different temperatures in different areas!
//
// 2 sensor (287B2275D0013CEC - blue wires):
// 37.0 on medical thermometer -> 36.7 on sensor
// Boiling water (100) -> 98.25, but water in boiling pan can have different temperatures in different areas!
const PROGMEM DeviceAddress kSensorAddress1  = {0x28, 0xB6, 0x16, 0x75, 0xD0, 0x01, 0x3C, 0xA2};
constexpr float             kRawHigh1        = 97.25;
constexpr float             kReferenceHigh1  = 100.0;
constexpr float             kRawLow1         = 35.92;  // 36.31
constexpr float             kReferenceLow1   = 36.7;   // 37.0
constexpr float             kRawRange1       = kRawHigh1 - kRawLow1;
constexpr float             kReferenceRange1 = kReferenceHigh1 - kReferenceLow1;
const PROGMEM DeviceAddress kSensorAddress2  = {0x28, 0x7B, 0x22, 0x75, 0xD0, 0x01, 0x3C, 0xEC};
constexpr float             kRawHigh2        = 98.25;
constexpr float             kReferenceHigh2  = 100.0;
constexpr float             kRawLow2         = 36.7;
constexpr float             kReferenceLow2   = 37.0;
constexpr float             kRawRange2       = kRawHigh2 - kRawLow2;
constexpr float             kReferenceRange2 = kReferenceHigh2 - kReferenceLow2;

bool
AreSensorAddressesEqual(DeviceAddress const& l, DeviceAddress const& r)
{
    for (uint8_t i = 0; i < 8; ++i) {
        if (l[i] != pgm_read_byte(&r[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace

ThermoSensors::ThermoSensors(uint8_t pin)
  : pin_{pin}
  , oneWire_{}
  , sensors_{}
  , last_temperatures_{kInvalidTemperature, kInvalidTemperature}
{
}

void
ThermoSensors::Setup()
{
    oneWire_.begin(pin_);
    sensors_.setOneWire(&oneWire_);
    sensors_.begin();

    Serial << F("Found ") << sensors_.getDeviceCount() << F(" thermal sensors.\n");
    if (sensors_.getDeviceCount() < kNumOfSensors_) {
        Serial << F("ERROR: expected number of sensors is ") << kNumOfSensors_ << endl;
    }

    for (int i = 0; i < kNumOfSensors_; ++i) {
        if (sensors_.getAddress(addresses_[i], i)) {
            sensors_.setResolution(addresses_[i], resolution_);
        }
        else {
            Serial << F("Unable to get address for Device ") << i << endl;
        }
    }

    sensors_.setResolution(resolution_);
    sensors_.setWaitForConversion(false);
    sensors_.requestTemperatures();
}

void
ThermoSensors::Loop()
{
    static uint32_t last_reading_time{0};
    auto            now = millis();
    if ((now - last_reading_time) < conversion_timeout_) {
        return;
    }
    last_reading_time = now;

    last_temperatures_[0] = ConvertByCalibration(sensors_.getTempC(addresses_[0]), addresses_[0]);
    last_temperatures_[1] = ConvertByCalibration(sensors_.getTempC(addresses_[1]), addresses_[1]);
    sensors_.requestTemperatures();
}

void
ThermoSensors::GetTemperatures(float (&temperatures)[2]) const
{
    temperatures[0] = last_temperatures_[0];
    temperatures[1] = last_temperatures_[1];
}

float
ThermoSensors::ConvertByCalibration(float T, DeviceAddress const& sensor_address) const
{
    if (AreSensorAddressesEqual(sensor_address, kSensorAddress1)) {
        return (((T - kRawLow1) * kReferenceRange1) / kRawRange1) + kReferenceLow1;
    }
    else if (AreSensorAddressesEqual(sensor_address, kSensorAddress2)) {
        return (((T - kRawLow2) * kReferenceRange2) / kRawRange2) + kReferenceLow2;
    }
    else {
        return T;
    }
}
