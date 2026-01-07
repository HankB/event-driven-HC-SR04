# event-driven-HC-SR04

**This branch is a WIP to work with GPIOD V2.2 which ships with Debian Trixie and derivatives. If you are working with an older version of Debian (or RPiOS) you will want to look at the `GPIOD_V1.6` branch.**

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

* 2026-01-07 The C++ version is working.
    * It needs more thorough testing
    * It really wants to be refactored as a proper C++ program.
* 2025-12-31 The version that uses libgpiod V2.2 will require a complete rewrite.

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

## 2025-01-09 I broke it

Back to testing/dev. `hcsr04_distancepy` starts and hangs. The Python version I used previously still works so the H/W connection is good. Checking GPIO assignments - look good. Rebuilt and try again. One reading on the first try before hanging. That seems repeatable, sometimes. Clearly a recent change to the logic is not effective. :-/

## 2025-01-09 I fixed it

Needed to resend the pulse when `gpiod_line_event_wait()` returns zero.

## 2025-01-09 ARGGG still broke

Python code reports more or less correct distance most of the time (with some occasional wildly inaccurate readings: 268 vs. 22.3)

## 2025-01-10 add debug output

Code seems to be missing the rising edge. Will look at timing ov various events.

## 2025-01-14

After much struggling (and adding debug statements), the code was tested on another host (Pi 4B) running RpiOS 11 and generally found to be working. Further testing seems to indicate that the 32 bit kernel is relatred to the issue. Measurements worked as desired on a Pi 4B and Zero 2W with the same 32 bit Bookworm installation on the SD card. The difference is that the Zero 2W and 4B boot a 64 bit kernel and the Zero W boots a 32 bit kernel.

```text
hbarta@nbw:~ $ uname -a
Linux nbw 6.6.62+rpt-rpi-v7 #1 SMP Raspbian 1:6.6.62-1+rpt1 (2024-11-25) armv7l GNU/Linux
hbarta@nbw:~ $ 
```

```text
hbarta@canby:~ $ uname -a
Linux canby 6.1.21-v8+ #1642 SMP PREEMPT Mon Apr  3 17:24:16 BST 2023 aarch64 GNU/Linux
hbarta@canby:~ $ 
```

After moving the card and HC-SR04 back to the Zero to confirm that the 32 bit kernel causes the issue, the program worked as desired. My first inclination is to suspect that I had somehow swapped the connections (and inadvertently "fixed" things.) And to be sure, the alternate Python program no longer works. But I *did* try the Python code on the 64 bit kernels and it did work.

Tested at ~105 inches and results were generally satisfactory. Tested *in situ* and results were questionable. However, the unit was operating at about -10°C which is outside the stated range of the unit. At this point, the results are considered satisfactory.

## errata

* 2026-01-06 TIL that Linux seems not to be able to provide microsecond timing delay. See `microseconds.cpp` for an example.
