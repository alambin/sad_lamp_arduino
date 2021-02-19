// Note! You should define "throw" keyword as nothing.
// Otherwise ArduinoSTL will generate tons of warnings that this keyword is deprecated
//#define COUT_OUTPUT
#ifdef COUT_OUTPUT
#define throw(...)
#include <ArduinoSTL.h>
#endif

#include "lamp_controller.h"

#include "doutpwm.h"
#include "fan.h"
#include "led.h"
#include "led_driver.h"
#include "potentiometer.h"
#include "serial_command_reader.h"
#include "timer.h"
#include "utils.h"

namespace
{
LampController lamp_controller;

Led led(LED_BUILTIN);
// FanPWM        fan(3, Pwm::PWMSpeed::HZ_31372);
// DoutPwm       dout_pwm(FAN1_PIN, FAN2_PIN);
// Potentiometer potentiometer(POTENTIOMETER_PIN, 10);
}  // namespace

void
setup()
{
    SerialCommandReader::instance();  // Init serial port by creating instance of  SerialCommandReader
    lamp_controller.setup();
}

// Blink 5 times signalling end of loop
void
blink_end_of_loop()
{
    constexpr char num_of_blinks{5};
    for (char i = 0; i < num_of_blinks; ++i) {
        led.turn_on(true);
        delay(500);
        led.turn_on(false);
        delay(500);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
print_performance(uint32_t delta)
{
    static bool     measurements_started = false;
    static uint32_t runc_count           = 0;
    static uint32_t avg_delta            = 0;

    // Start measuring only from 100th iteration
    if ((++runc_count >= 100) || measurements_started) {
        if (!measurements_started) {
            runc_count           = 1;
            measurements_started = true;
        }

        avg_delta += delta;
    }

    if ((runc_count & 0x7FFF) == 0) {
        Serial.print(F("Main loop execution time: "));
        Serial.print(avg_delta / runc_count);
        Serial.print(F(" us"));
        Serial.println();

        avg_delta  = 0;
        runc_count = 0;
    }
}

// the loop function runs over and over again forever
void
loop()
{
    auto start_time = micros();
    lamp_controller.loop();
    // auto delta = micros() - start_time;
    // print_performance(delta);

    // pwm_up_with_blinking(10);
    // test_dout_30hz_pwm_duty_cycles();

    // TODO:
    // V 1. Implement FAN PWM using DOUT. For now keep 10 steps and 3 Hz.
    // V 2. Make num_of_pwm_steps and pwm_frequency configurable via USB. May be we will need to use commands longer
    //    than 1 char (strings would be very flexible). Store these variables in EEPROM. So, user later can configure
    //    it without rebuilding.
    // 3. Buy 1-way mosfet module without optocoupler. Try to use and measure noise. If on 32 kHz it will not make
    //    much noise, buy 4-way key. Try to find WITH PROTECTIVE DIODES.
    //    Status: ordered, waiting for arrival. Actually ordered module is shit, because to fully oper transistor you
    //    need 10V, but Arduino gives only 5V. You can try to replace default mosfet with, ex. IRL3705N
    //    Result: NO NOISE on 32 kHz, as expected! Not sure if I can find 4-way no-isolated board.
    // V 4. Try to find high speed optocoupler so that you can replace them on my boards. Need 10 Mbit/s speed and
    //    Vcc (output voltage) up to 15V.
    //    Do NOT look at https:
    //    aliexpress.ru/item/32914498721.html?spm=a2g0o.productlist.0.0.4b8a735dX01mxU&algo_pvid=db4f0cfc-d144-4e65-af56-f3bb5a12ab00&algo_expid=db4f0cfc-d144-4e65-af56-f3bb5a12ab00-4&btsid=0b8b034116094162831996129ef754&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_
    //    It support only inputs on 12/24V. Output is always 5V (maximum allowed on 6N137 is 7V).
    //    Results:
    //    1) 5 Mb, 0-20V: https://datasheet.octopart.com/HCPL-2201-Avago-datasheet-8212218.pdf
    //       BUT it will have inverted output
    // V 5. Buy circle resistor (Potentiometer). How to mount it to the metal plate? Prefere those one, which has
    //    fixed "0" position. Should you connect it to Arduino or somehow to LED driver?
    // 6. Try to find replacement LED (citizen? cree?) for 2.2 A, the same voltage, but 5500-5700K.
    //    Refer to LED holder description to get supported LEDs
    //    Status:
    //    1) Deal with Neon, ordered 2x 5000K LEDs
    //    2) Contact on Alibaba to order/produce following items
    //       https://www.alibaba.com/product-detail/Citizen-COB-CLU048-Series-LED-MODULE_62135453445.html
    //       https:  //
    //       russian.alibaba.com/product-detail/photographic-lighting-citizen-same-size-clu048-xl-28-28-24-200w-300w-cri95-5600k-us-bridgelux-3-years-ce-rohs-lm-80-62555834989.html?spm=a2700.8699010.normalList.2.5cab32e0seliz3&s=p
    //       https:  //
    //       russian.alibaba.com/product-detail/led-cob-clu048-1818-for-150-200w-led-high-bay-or-led-flood-light-made-in-japan-28x28mm-ra-70-80-90-cct-3000-4000-5000-5700k-62361111010.html?spm=a2700.8699010.normalList.56.5cab32e0seliz3
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test of potenciometer readings and filtering
/*
#include "Potentiometer.h"
Potentiometer potentiometer(A0, 10);

void
testReadingsOfResistor()
{
    uint16_t rotat     = analogRead(A0) / 4;
    uint16_t inv_rotat = 255 - rotat;
    analogWrite(3, inv_rotat);

    // Serial.print("rotat = ");
    // Serial.print(rotat);
    // Serial.print("; inv_rotat = ");
    // Serial.println(inv_rotat);
    // delay(500);
}

void
setup()
{
    Serial.begin(9600);
    potentiometer.setup();

    // Serial.println('NotFiltered, Median3 ');

    // pinMode(3, OUTPUT);
    // TCCR2B &= B11111000;
    // TCCR2B |= B00000011;
    // digitalWrite(3, HIGH);

    pinMode(A0, INPUT);
}

void
loop()
{
    potentiometer.loop();
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test functions
/*
void
setup()
{
  fan.setup();
}

void
loop()
{
}

void
PWMFanTest()
{
    led.turn_on(false);

    fan.set_speed(0);  // 0%
    delay(10000);      // 10s pause

    led.turn_on(true);
    fan.set_speed(255);  // 100%
    delay(10000);        // 10s pause

    led.turn_on(false);
    fan.set_speed(64);  // 25%
    delay(5000);        // 5s pause

    led.turn_on(true);
    fan.set_speed(192);  // 75%
    delay(5000);         // 5s pause

    led.turn_on(false);
    fan.set_speed(128);  // 50%
    delay(5000);         // 5s pause
}

void
blink_n_times(int N)
{
    for (char i = 0; i < N; ++i) {
        led.turn_on(true);
        delay(500);
        led.turn_on(false);
        delay(500);
    }
}

// Results of measuring volts on fan in case of different PWM frequency and different duty cycle
// Small fan, small key, 30 Hz
// 1 - 0.8V
// 2 - 2.5V
// 3 - 3.5
// 4 - 4.5
// 5 - 5.5
// 6 - 6.5
// 7 - 7.7
// 8 - 8.6
// 9 - 9.7
// 10 - 10.8
// 11 - 11.8

// Small fan, small key, 122 Hz
// 1 - 0.4 (not running)
// 2 - 2.7 (not running)
// 3 - 3.7 (not running)
// 4 - 4.7 (not running)
// 5 - 5.9 STARTED
// 6 - 7
// 7 - 8
// 8 - 9
// 9 - 10
// 10 - 11.1 Still noise
// 11 - 12   No noise

// Small fan, small key, 490 Hz
// 1 - 0.5
// 2 - 3.8
// 3 - 4.9
// 4 - 5.9 STARTED
// 5 - 6.8
// 6 - 7.8
// 7 - 8.8
// 8 - 9.8
// 9 - 10.8
// 10 - 11.6 Still noise
// 11 - 12   No noise

// Small fan, small key, 4 kHz
// 1 - 0.3
// 2 - 8.9
// 3 - 9.9
// 4 - 10.7 Still noise
// 5 - 11.4 No noise
// 6 and later- 12

// New n-channel key module. Small fan. 32 kHz. NO NOISE AT ALL !!!
// 1 - 0.33
// 2 - 3.1
// 3 - 4.1
// 4 - 5.7
// 5 - 6.9
// 6 - 7.95
// 7 - 8.9
// 8 - 9.7
// 9 - 10.5
// 10 - 11.2
// 11 - 11.9

// New n-channel key module. Small fan. 32 kHz. Doesnt start. No power on fan. Idk why
// Same but 4 kHz - fan starts, but on high duty and with noise. Even at max PWM it gives max 9.7V to fan

// Test PWM by changing duty cycle in steps "numOfGrades" times from 0% to 100%
void
pwm_up_with_blinking(char numOfGrades)
{
    const int deltaPWM = 255 / numOfGrades;

    for (char step = 0; step <= numOfGrades; ++step) {
        fan.set_speed(step * deltaPWM);
        blink_n_times(step + 1);
        delay(5000);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Test PWM generated by digital output. So, we can test very low frequencies.
// Ex. lowest possible frequency on embedded PWM is 30 hz * 256 steps = 7,.7 kBit/s
// Observations:
// 1s - noise only from fan, working on high speed. No whinning
// 0.5 - same
// 1/4 - same
// 1/8 - same. Но частые включения уже начинают слышаться, как "скрежет"
// 1/16 - same. В принципе, если лампа будет стоять далеко, то вентиляторов будет не слышно
// 1/20 - с закрытыми глазами тяжело отличить звут от 100% работающего вентилятора и от PWM 2 * 1/20 s
//        в близи шума вроде как даже меньше стало
// 1/32 - шум во включенном состоянии, слышимый ранее, превратился в отчетливый скрежет.
//        Он, видимо, состоит из звуков включения вентилятора каждый раз, когда подается питание
// 1/40 - отличить начало PWM уже стало чууть-чуть проще
// 1/64 - same
// 1/128 - Whinning started
void
test_DOUT_pwm_frequencies()
{
    digitalWrite(3, HIGH);
    delay(5000);

    const uint16_t frequency_hz = 30;
    const uint16_t period_ms    = 1000 / frequency_hz;  // Half period
    for (uint32_t i = 0; i < (uint32_t)-1; ++i) {
        led.turn_on(true);
        digitalWrite(3, HIGH);
        delay(period_ms);
        led.turn_on(false);
        digitalWrite(3, LOW);
        delay(period_ms);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Малый вентилятор:
// 1c - работает нормально, но на малых процентах вентилятор не двигается.
//      А на больших он слишком инерционный, чтобы реагировать на отключения питания.
// 30 Гц с 3 под-периодами работает нормально. Это 3 Гц. Слышны отдельные периоды включения-отключения. Довольно тихо
// 100 Hz / 10 (10 Hz) Слышно вроде немного громче, но тоже малозаметно.
// 300 Гц с 30 под-периодами (30 Гц). Слышно гораздо сильнее.
// 600 Hz / 60 sub-periods (60 Hz). Шум еще сильнее
// 6000 Hz / 600 - писк
//
// Вентилятор 200мм:
// Довольно заметно "щелкает" даже на 3 Гц (10 шагов). Плюс, слышны его усилия, чтобы раскрутиться
// (это уже не щелчки, а звук раскручивающейся крыльчатки)
void
test_dout_30hz_pwm_duty_cycles()
{
    static char     state{0};
    static uint32_t start_time{0};
    uint32_t        delta_ms{start_time - millis()};
    static uint8_t  current_duty{0};

    dout_pwm.loop();

    switch (state) {
    case 0:
        digitalWrite(3, HIGH);
        delay(5000);
        digitalWrite(3, LOW);
        state      = 1;
        start_time = millis();
        break;
    case 1:
        if (delta_ms >= 5000) {
            start_time += 5000;  // Adjust start time to avoid overflow of "delta"

            ++current_duty;
            if (current_duty > num_of_pwm_steps) {
                current_duty = 0;
            }

            DebugPrint("New duty = ");
            DebugPrintln(current_duty);

            dout_pwm.set_duty(current_duty);
        }
        break;
    }
}
*/