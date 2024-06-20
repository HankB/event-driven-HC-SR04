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

static const int echo = 23;                     // GPIO # for echo input
static const struct timespec timeout = {10, 0}; // run for 10s and exit

int main(int argc, char **argv)
{
    float distance;                // distance we measure
    static const int trigger = 24; // GPIO # for trigger output

    // printf("clocks/sec %ld\n", CLOCKS_PER_SEC);
    //  working with clocks/sec 1,000,000 on RpiOS 32 bit, Pi 3B
    printf("init trigger\n");
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     trigger, 0, false,
                                     "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_get_value");
        return 1;
    }

    send_pulse(trigger); // send the trigger pulse
    printf("registering event\n");
    rc = gpiod_ctxless_event_monitor("/dev/gpiochip0",
                                     GPIOD_CTXLESS_EVENT_BOTH_EDGES,
                                     echo, false, "consumer",
                                     &timeout, 0, event_cb, 0);
    printf("event registered\n");
    if (rc < 0)
        perror("gpiod_ctxless_get_value");

    printf("Hello World, rc is %d\n", rc);
    /*
    while (1)
    {
        send_pulse(trigger); // send the trigger pulse
        usleep(500000);      // wait half second (needed?)
        distance = read_distance();
        printf("distance %f\n", distance);
    }
    */
}

/*
 * At present send_pulse() not only sends the pulse but also polls for the result. for the
 */
static void send_pulse(int gpio)
{
    printf("sending pulse\n");
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     gpio, 1, false,
                                     "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_get_value");
        return;
    }
    usleep(10); // 10 microsecond (minimim) sleep
    rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                 gpio, 0, false,
                                 "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_get_value");
    }
}

static float read_distance(void)
{
    // 39.5 empirically determined by taking readings at 6", 12" and 18"
    // to convert the readings to inches.
    /*
    float distance = ((float)(end_echo - start_echo)) / 39.5;
    printf("%d %d %d ", int_count[0], int_count[1], int_count[2]);
    return distance;
    */
    printf("billions and billions ...\n");
    return 9999999.0;
}

int event_cb(int i, unsigned int j, const struct timespec *ts, void *unused)
{
    static struct timespec start = {0.0};

    printf("i: %d, j:%d at %ld.%9.9ld\n", i, j, ts->tv_sec, ts->tv_nsec);

    if (GPIOD_CTXLESS_EVENT_CB_RISING_EDGE == i)
    {
        start = *ts;
    }
    else if (GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE == i)
    {
        long nsec = ts->tv_nsec - start.tv_nsec;
        printf( "delta-T:%ld\n", nsec);
    }

    return 0;
}
