# event-driven-HC-SR04

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

Color codes are relative to the DuPont jumpers I used. The resistor divider is 1.5K Echo to orange and 3K orange to ground.

## Device details

* <https://www.digikey.com/htmldatasheets/production/1979760/0/0/1/hc-sr04.html> " we suggest to use over 60ms measurement cycle, in order to prevent trigger signal to the echo signal."
* <https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/> "This sensor reads from 2cm to 400cm (0.8inch to 157inch) with an accuracy of 0.3cm (0.1inches), ..."

## Status

* 2024-06-27 Testing at greater distance. 50 consecutive readings are all over the place.
* 2024-06-24 Code seems to be working well, albeit only tested in a Pi 3B and at distances ranging from 1-2 feet. Need to test on a Pi Zero and at other distances. It would be interesting to try on a Pi 5 as well.
* 2024-11-23 Back to working on this. Need to do more testing and perhaps calibration.

## Troubleshooting notes

### 2024-06-22 

* Timing. Study the description at <https://thepihut.com/blogs/raspberry-pi-tutorials/hc-sr04-ultrasonic-range-sensor-on-the-raspberry-pi> and see that they delay 2s after configuring the trigger output. Tried - no help. But will leave in. <https://www.raspberrypi-spy.co.uk/2012/12/ultrasonic-distance-measurement-using-python-part-1/> suggests 0.5s so I will go with this. 
* Interesting, inserting a 1s delay between registering the event and sending the pulse results in a timeout in the event monitor.
* More tweaking of timing. Giving up on the contextless operations for now as the events are happening before the trigger pulse is sent. (#457226f)

### 2024-06-22 

* `libgpiod` events generally going well. Occasionally it seems that the rising edge of the pulse has been missed so the code has been restructured and will retry in this situation (sending another trigger pulse and monitoring for events on the echo input.)

## Local dev notes

I need to record where I'm working on this so if I need to set it aside and come back months later, I can resume on the correct host.

### 2024-11-32 environment

* Test/target host Raspberry Pi Zero W Rev 1.1 running RpiOS Bookworm and fully updated (HC-SR04 connected)
* `libgpiod` 1.6.3
* `canby`

* Dev host: Raspberry Pi 4 Model B Rev 1.5 (2GB) running RpiOS Bullseye 32 bit fully up to date
* `libgpiod` 1.6.2
* `nbw` (New Brandywine)

Editing using VS code/SSH from desktop. Typical commands (on `canby`)

```text
cd Programming/event-driven-HC-SR04/
gcc -Wall -o hcsr04_distance hcsr04_distance.c  -l gpiod
scp hcsr04_distance nbw:bin
```
