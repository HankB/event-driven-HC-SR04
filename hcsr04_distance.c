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

static int send_pulse(struct gpiod_line *);
static float read_distance(void);
int event_cb(int i, unsigned int j, const struct timespec *ts, void *unused);

// static const int echo = 23;                           // GPIO # for echo input
// static const int trigger = 24;                        // GPIO # for trigger output
//  static const struct timespec timeout = {1, 0};        // monitor events for up to 1s and exit
static const char *cnsmr = "HC-SR04";                 // declare a consumer to use for calls
static const char *GPIO_chip_name = "/dev/gpiochip0"; // name of trigger GPIO (output)
static const char *trigger_name = "GPIO24";           // name of trigger GPIO (output)
static const char *echo_name = "GPIO23";              // name of echo GPIO input

static struct timespec start = {0.0};  // start of echo pulse
static struct timespec finish = {0.0}; // end of echo pulse

int main(int argc, char **argv)
{
    // float distance; // distance we measure

    // need to open the chip first
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_chip_name);
    if (chip == NULL)
    {
        perror("gpiod_chip_open()");
        return 1;
    }

    // acquire & configure GPIO 24 (trigger) for output

    struct gpiod_line *trigger_line;

    trigger_line = gpiod_chip_find_line(chip, trigger_name);
    if (trigger_line == NULL)
    {
        perror("gpiod_chip_find_line(trigger_name)");
        gpiod_chip_close(chip);
        return 1;
    }

    if (trigger_line == NULL)
    {
        perror("gpiod_chip_find_line(trigger_name)");
        gpiod_chip_close(chip);
        return 1;
    }

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

    // acquire & configure GPIO 23 (echo) for input

    struct gpiod_line *echo_line;

    echo_line = gpiod_chip_find_line(chip, echo_name);
    if (echo_line == NULL)
    {
        perror("gpiod_chip_find_line(echo_name)");
        gpiod_chip_close(chip);
        return 1;
    }
    if (echo_line == NULL)
    {
        perror("gpiod_chip_find_line(echo_name)");
        gpiod_chip_close(chip);
        return 1;
    }

#if 0
    const struct gpiod_line_request_config read_config =
        {cnsmr,
         GPIOD_LINE_REQUEST_DIRECTION_INPUT,
         0};

    rc = gpiod_line_request(echo_line,
                            &read_config,
                            0);
    if (rc < 0)
    {
        perror("               gpiod_line_request(echo_line)");
        gpiod_line_release(echo_line);
        gpiod_chip_close(chip);
    }

    printf("===== line configured for read \n");
    report_line_attributes(echo_line, cnsmr);
    printf("============\n\n");

    sleep(1); // allow input, output to settle
#endif

    ////////////////////////////////////////////////
    rc = gpiod_line_request_both_edges_events(echo_line, cnsmr);
    printf("%d = gpiod_line_request_both_edges_events()\n", rc);
    if (0 > rc)
    {
        perror("               gpiod_line_request_both_edges_events(echo_line)");
        gpiod_line_release(trigger_line);
        gpiod_line_release(echo_line);
        gpiod_chip_close(chip);
        return -1;
    }

    rc = send_pulse(trigger_line); // send the trigger pulse
    printf("%d = send_pulse()\n", rc);
    if (0 != rc)
    {
        perror("send_pulse(trigger_line)");
        gpiod_line_release(trigger_line);
        gpiod_line_release(echo_line);
        gpiod_chip_close(chip);
        return -1;
    }

    // gpiod_line_event_wait() seems not required unless a timeout is needed

    while (true)
    {
        const struct timespec timeout = {0L, 1000000L}; // 1000000ns, 0s
        rc = gpiod_line_event_wait(echo_line, &timeout);
        printf("%d = gpiod_line_event_wait()\n", rc);
        if (rc < 0)
        {
            perror("               gpiod_line_event_wait(echo_line)");
            gpiod_line_release(trigger_line);
            gpiod_line_release(echo_line);
            gpiod_chip_close(chip);
            return -1;
        }
        struct gpiod_line_event event;
        rc = gpiod_line_event_read(echo_line, &event);
        printf("%d = gpiod_line_event_read()\n", rc);
        if (rc < 0)
        {
            printf("event %d at %ld s, %ld ns\n", event.event_type, event.ts.tv_sec, event.ts.tv_nsec);
            perror("               gpiod_line_event_read(gpio_11, &event)");
        }
        else
        {
            printf("event %d at %ld s, %ld ns\n",
                   event.event_type, event.ts.tv_sec, event.ts.tv_nsec);
            switch (event.event_type)
            {
            case GPIOD_LINE_EVENT_RISING_EDGE:
            {
                start = event.ts;
                break;
            }
            case GPIOD_LINE_EVENT_FALLING_EDGE:
            {
                finish = event.ts;
                long int dt = finish.tv_nsec - start.tv_nsec;
                printf("delta-T: %ld %ld %ld \n", finish.tv_nsec, start.tv_nsec, dt);
                gpiod_line_release(trigger_line);
                gpiod_line_release(echo_line);
                gpiod_chip_close(chip);
                printf("distance %f\n", read_distance());
                return 0;
            }
            }
        }
    }
    ////////////////////////////////////////////////

#if 0


    distance = read_distance();
    printf("Hello World, rc is %d, distance %f\n", rc, distance);
#endif
    gpiod_line_release(trigger_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
}

static int send_pulse(struct gpiod_line *line)
{
    printf("sending pulse\n");
    int rc = gpiod_line_set_value(line, 1);
    if (rc < 0)
    {
        perror("               gpiod_line_set_value(line,1)");
        return 1;
    }
    usleep(10);
    rc = gpiod_line_set_value(line, 0);
    if (rc < 0)
    {
        perror("               gpiod_line_set_value(line,0)");
        return 1;
    }
    return 0;
}

// read_distance() will calculate the distance in feet based on time
//  stamps collected during event monitoring
static float read_distance(void)
{
    float pulse_width = (float)(finish.tv_nsec - start.tv_nsec) / 1000000000;
    // TODO handle rollover of tv_sec between readings
    float distance = pulse_width * 1100 / 2.0;
    return distance;
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