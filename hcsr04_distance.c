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

static const char *cnsmr = "HC-SR04";                 // declare a consumer to use for calls
static const char *GPIO_chip_name = "/dev/gpiochip0"; // name of trigger GPIO (output)
static const char *trigger_name = "GPIO24";           // name of trigger GPIO (output)
static const char *echo_name = "GPIO23";              // name of echo GPIO input

static struct timespec start = {0.0};  // start of echo pulse
static struct timespec finish = {0.0}; // end of echo pulse

int main(int argc, char **argv)
{
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

    // register to read events
    rc = gpiod_line_request_both_edges_events(echo_line, cnsmr);
    if (0 > rc)
    {
        perror("               gpiod_line_request_both_edges_events(echo_line)");
        gpiod_line_release(trigger_line);
        gpiod_line_release(echo_line);
        gpiod_chip_close(chip);
        return -1;
    }

    rc = send_pulse(trigger_line); // send the trigger pulse
    if (0 != rc)
    {
        perror("send_pulse(trigger_line)");
        gpiod_line_release(trigger_line);
        gpiod_line_release(echo_line);
        gpiod_chip_close(chip);
        return -1;
    }

    while (true)
    {
        const struct timespec timeout = {0L, 1000000L}; // 1000000ns, 0s
        rc = gpiod_line_event_wait(echo_line, &timeout);
        //  printf("%d = gpiod_line_event_wait()\n", rc);
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
        //  printf("%d = gpiod_line_event_read()\n", rc);
        if (rc < 0)
        {
            perror("               gpiod_line_event_read(gpio_11, &event)");
        }
        else
        {
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

    gpiod_line_release(trigger_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
}

static int send_pulse(struct gpiod_line *line)
{
    //  printf("sending pulse\n");
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
