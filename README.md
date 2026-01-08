# event-driven-HC-SR04

The `main` breanch is the next generation which uses the V2.2 lingpiod and is written in C++. (See <https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/tree/main/60-Capstone_C%2B%2B> for the rationale for the switch to C++.) V2.2 is what ships with Debian Trixie and derivatives. This is also the version that is available to install on Ubuntu 25.10.

Event driven HC-SR04 measurement on a Raspberry Pi using C.

This code uses the `libgpiod` library and time stamped events to measure the length of the echo pulse. See <https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/blob/main/C_blinky/event_drive.c> for simple code that demonstrates this.

## Motivation

Use an HC-SR04 to measure distance with a minimum of computer resources (targeted at a Pi 0). This argues for a compiled language that uses an interrupt to measure the return pulse time. (There are a lot of existing projects on Github that use Python and/or C/C++ and which poll for the return pulse, tieing up the single core available in the Pi 0,)

* <https://www.handsontec.com/dataspecs/HC-SR04-Ultrasonic.pdf> user guide.

## Inspiration

* <https://github.com/phil-lavin/raspberry-pi-gpio-interrupt/blob/master/gpio-interrupt.c>
* <https://gist.github.com/keriszafir/37d598d6501214da58e0> (derived from previous)
* <https://github.com/JayChuang/hc-sr04/blob/master/hc-sr04.c> HC-SR04 project that uses interrupts!

The last one provides a complete solution but seems a little involved to produce and would require rebuilding each time the kernel is upgraded. Initial efforts will be to write something simpler based on the Phil Lavin example.

## H/W configuration

S/W is targeted for a Pi Zero and development is performed on a Pi 3B with the HC-SR04 connected as follows:

|cable|signal|pin|GPIO|notes|
|---|---|---|---|---|
|green|GND|20|
|orange|echo|18|23|Resistor divider at sensor end.|
|yellow|trigger|16|24|
|red|VCC|4|

Color codes are relative to the DuPont jumpers I used. The resistor divider is 1.5K Echo to orange and 3K orange to ground. (I don't recall if I used the same for the V2.2 version but retained the same 2:1 ratio.)

## Device details

* <https://www.digikey.com/htmldatasheets/production/1979760/0/0/1/hc-sr04.html> " we suggest to use over 60ms measurement cycle, in order to prevent trigger signal to the echo signal."
* <https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/> "This sensor reads from 2cm to 400cm (0.8inch to 157inch) with an accuracy of 0.3cm (0.1inches), ..."

## Status

* 2026-01-07 The C++ version is working.
    * It needs more thorough testing
    * It really wants to be refactored as a proper C++ program.
* 2025-12-31 The version that uses libgpiod V2.2 will require a complete rewrite.

## Troubleshooting notes

Most of these notes were relavent to the V1.6 branch and are removed here (ewxcept for the Timing comment.)

### 2024-06-22 Timing

* Timing. Study the description at <https://thepihut.com/blogs/raspberry-pi-tutorials/hc-sr04-ultrasonic-range-sensor-on-the-raspberry-pi> and see that they delay 2s after configuring the trigger output. Tried - no help. But will leave in. <https://www.raspberrypi-spy.co.uk/2012/12/ultrasonic-distance-measurement-using-python-part-1/> suggests 0.5s so I will go with this. 

## errata

* 2026-01-06 TIL that Linux seems not to be able to provide microsecond timing delay. See `microseconds.cpp` for an example.
* 2026-01-06 Some of the most verbose output levels (`debug_lvl` >= 2) result in delays that cause the event capture to be postponed until well after the falling edge of the echo pulse. Despite this, the timing information associated with the event is solid and results are not affected.
