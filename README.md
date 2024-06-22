# interrupt-driven-HC-SR04

Interrupt driven HC-SR04 measurement on a Raspberry Pi using C.

The `libgpiod` branch uses the libgpiod library and uses time stamped events to measure the length of the echo pulse. See <https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/blob/main/C_blinky/event_drive.c> for simple code that demonstrates this.

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

color codes are relative to the DuPont jumpers I used. The resistor divider is 1.5K Echo to orange and 3K orange to ground.

## Status

* 2024-06-22 Working and using the `libgpiod` event facility. Results not accurate.
* 2024-06-13 fiddling with polled version. Planning to implement interrupts in a feature branch
* 2024-06-13 first cut at using an ISR. I don't understand the results. There seem to be many fewer clock counts than with polling. Perhaps `clock()` is not suitable to use in an ISR.
* 2024-05-01 Working measurement of sorts using empirically determined conversion factor. And polling.
* 2024-04-30 (First commit of `hcsr04_distance.c`) Send long trigger pulse to verify GPIO write. Can also see a disturbance in the echo pin using a DVM.

## Troubleshooting notes

### 2024-06-22 

* Timing. Study the description at <https://thepihut.com/blogs/raspberry-pi-tutorials/hc-sr04-ultrasonic-range-sensor-on-the-raspberry-pi> and see that they delay 2s after configuring the trigger output. Tried - no help. But will leave in. <https://www.raspberrypi-spy.co.uk/2012/12/ultrasonic-distance-measurement-using-python-part-1/> suggests 0.5s so I will go with this. 
* Interesting, inserting a 1s delay between registering the event and sending the pulse results in a timeout in the event monitor.
* More tweaking of timing. Giving up on the contextless operations for now as the events are happening before the trigger pulse is sent. (#457226f)
