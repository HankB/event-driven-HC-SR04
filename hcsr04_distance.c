/*
Measure distance using an HC-SR04 sensor using Interrupts.

Build:
    gcc -Wall -o hcsr04_distance hcsr04_distance.c  -l gpiod
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <gpiod.h>

static void send_pulse(int gpio);
static float read_distance(void);
int event_cb(int i, unsigned int j, const struct timespec *ts, void *unused);

static const int echo = 23;                    // GPIO # for echo input
static const struct timespec timeout = {1, 0}; // run for 1s and exit

int main(int argc, char **argv)
{
    float distance;                // distance we measure
    static const int trigger = 24; // GPIO # for trigger output

    // init the ytihhrt output
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     trigger, 0, false,
                                     "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_set_value");
        return 1;
    }

    send_pulse(trigger); // send the trigger pulse
    rc = gpiod_ctxless_event_monitor("/dev/gpiochip0",
                                     GPIOD_CTXLESS_EVENT_BOTH_EDGES,
                                     echo, false, "consumer",
                                     &timeout, 0, event_cb, 0);
    if (rc < 0)
        perror("gpiod_ctxless_event_monitor");
    distance = read_distance();
    printf("Hello World, rc is %d, distance %f\n", rc, distance);
}

/*
 * send pulse to initiate measurement
 */
static void send_pulse(int gpio)
{
    printf("sending pulse\n");
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     gpio, 1, false,
                                     "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_set_value");
        return;
    }
    usleep(10); // 10 microsecond (minimim) sleep
    rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                 gpio, 0, false,
                                 "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_set_value");
    }
}

static struct timespec start = {0.0};  // start of echo pulse
static struct timespec finish = {0.0}; // end of echo pulse

// read_distance() will calculate the distance based on time stamps collected
// by event_cb() 
static float read_distance(void)
{
    float pulse_width = (float)(finish.tv_nsec - start.tv_nsec) / 1000000000;
    // TODO handle rollover of tv_sec between readings
    float distance = pulse_width*1100/2.0;
    return distance;
}

// callback for GPIO transitions on the echo pin
int event_cb(int i, unsigned int j, const struct timespec *ts, void *unused)
{

    printf("i: %d, j:%d at %ld.%9.9ld\n", i, j, ts->tv_sec, ts->tv_nsec);

    if (GPIOD_CTXLESS_EVENT_CB_RISING_EDGE == i)
    {
        start = *ts;
    }
    else if (GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE == i)
    {
        finish = *ts;
        return 1; // will result in return of gpiod_ctxless_event_monitor()
    }
    else // timeout on monitor (set by struct timespec timeout)
    {
        return 1;
    }

    return 0;
}
