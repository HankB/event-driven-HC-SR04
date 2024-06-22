/*
Measure distance using an HC-SR04 sensor using event monitoring.

This iteration will eschew the contextless operations which seem to
produce inconsistent results.

Starting point will be some of the code from
https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/blob/main/line_IO/read_write.c

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

static const int echo = 23;                           // GPIO # for echo input
static const int trigger = 24;                        // GPIO # for trigger output
static const struct timespec timeout = {1, 0};        // monitor events for up to 1s and exit
static const char *cnsmr = "HC-SR04";                 // declare a consumer to use for calls
static const char *GPIO_chip_name = "/dev/gpiochip0"; // name of trigger GPIO (output)
static const char *trigger_name = "GPIO24";           // name of trigger GPIO (output)
static const char *echo_name = "GPIO23";              // name of echo GPIO input

// report line attributes (debug code)
void report_line_attributes(struct gpiod_line *line, const char *name)
{
    bool is_free = gpiod_line_is_free(line);
    printf("Is free        %s is %s\n", name, is_free ? "yes" : "no");
    bool is_requested = gpiod_line_is_requested(line);
    printf("Is requested   %s is %s\n", name, is_requested ? "yes" : "no");

    int bias = gpiod_line_bias(line);
    printf("Bias of        %s is %d\n", name, bias);

    const char *consumer = gpiod_line_consumer(line);
    printf("Consumer of    %s is %s\n", name, consumer);

    int direction = gpiod_line_direction(line);
    printf("Direction of   %s is %d\n", name, direction);

    bool used = gpiod_line_is_used(line);
    printf("Is             %s used %s\n", name, used ? "yes" : "no");

    int value = gpiod_line_get_value(line);
    printf("Value          %s is %d\n", name, value);
}

int main(int argc, char **argv)
{
    float distance; // distance we measure

    // need to open the chip first
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_chip_name);
    if (chip == NULL)
    {
        perror("gpiod_chip_open()");
        return 1;
    }

    // acquire & configure GPIO 24

    struct gpiod_line *trigger_line;

    trigger_line = gpiod_chip_find_line(chip, trigger_name);
    if (trigger_line == NULL)
    {
        perror("gpiod_chip_find_line(trigger_name)");
        gpiod_chip_close(chip);
        return 1;
    }
    printf("===== line found, unconfigured\n");
    report_line_attributes(trigger_line, trigger_name);
    sleep(2);

    if (trigger_line == NULL)
    {
        perror("gpiod_chip_find_line(trigger_name)");
        gpiod_chip_close(chip);
        return 1;
    }
    printf("===== line found, unconfigured\n");
    report_line_attributes(trigger_line, trigger_name);
    sleep(2);
    
    const struct gpiod_line_request_config write_config =
        {cnsmr,
         GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
         0};

    int rc = gpiod_line_request(trigger_line,
                                &write_config,
                                0);
    if (rc < 0)
    {
        perror("               gpiod_line_request(trigger_line)");
        gpiod_line_release(trigger_line);
        gpiod_chip_close(chip);
    }

    printf("===== line configured for write, default low\n");
    report_line_attributes(trigger_line, cnsmr);
    printf("============\n\n");

#if 0
    // init the trigger output
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     trigger, 0, false,
                                     "consumer", 0, 0);
    if (rc < 0)
    {
        perror("gpiod_ctxless_set_value");
        return 1;
    }

    // register to monitor events
    rc = gpiod_ctxless_event_monitor("/dev/gpiochip0",
                                     GPIOD_CTXLESS_EVENT_BOTH_EDGES,
                                     echo, false, "consumer",
                                     &timeout, 0, event_cb, 0);
    if (rc < 0)
        perror("gpiod_ctxless_event_monitor");

    sleep(1); // allow input, output to settle

    rc = gpiod_ctxless_get_value("/dev/gpiochip0",
                                 echo, false, "consumer");
    printf("echo %d\n", rc);

    send_pulse(trigger); // send the trigger pulse

    distance = read_distance();
    printf("Hello World, rc is %d, distance %f\n", rc, distance);
#endif
    gpiod_line_release(trigger_line);
    gpiod_chip_close(chip);
}

#if 0
/*
 * Callback for send_pulse() to delay 10ns.
 */
static void send_pulse_cb(void *unused)
{
    usleep(10); // 10 microsecond (minimim) sleep
}
/*
 * send pulse to initiate measurement. Callback will delay 10ns
 * And at the completion of gpiod_ctxless_set_value() the pulse will
 * turn off. I think.
 */
static void send_pulse(int gpio)
{
    printf("sending pulse\n");
    int rc = gpiod_ctxless_set_value("/dev/gpiochip0",
                                     gpio, 1, false,
                                     "consumer", send_pulse_cb, 0);
    if (rc < 0)
    {
        perror("send_pulse(): gpiod_ctxless_set_value");
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
    float distance = pulse_width * 1100 / 2.0;
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
#endif