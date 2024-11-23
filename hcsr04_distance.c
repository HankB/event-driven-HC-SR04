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

typedef enum
{
    read_line,
    write_line,
    monitor_line,
} line_function;

struct gpiod_line *init_GPIO(struct gpiod_chip *chip,
                             const char *name,
                             const char *context,
                             line_function function, int flags);

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
    trigger_line = init_GPIO(chip, trigger_name, cnsmr, write_line, 0);
    if (0 == trigger_line)
    {
        perror("               init_GPIO(trigger_line)");
        gpiod_chip_close(chip);
        return 1;
    }
    // acquire & configure GPIO 23 (echo) for input

    struct gpiod_line *echo_line;

    echo_line = init_GPIO(chip, echo_name, cnsmr, monitor_line, 0);

    if (0 == echo_line)
    {
        perror("gpiod_chip_find_line(echo_name)");
        gpiod_line_release(trigger_line);
        gpiod_chip_close(chip);
        return 1;
    }

    fprintf( stderr, "pulse seconds, distance inches\n");

    int reading_count = 0;
    bool need_pulse = true;
    while (reading_count < 50)
    {
        if (need_pulse)
        {
            usleep(60*1000); // delay 60 msec per recommendation
            int rc = send_pulse(trigger_line); // send the trigger pulse
            if (0 != rc)
            {
                perror("send_pulse(trigger_line)");
                gpiod_line_release(trigger_line);
                gpiod_line_release(echo_line);
                gpiod_chip_close(chip);
                return -1;
            }
            need_pulse = false;
        }

        const struct timespec timeout = {0L, 1000000L}; // 1000000ns, 0s
        int rc = gpiod_line_event_wait(echo_line, &timeout);
        if (rc < 0)
        {
            perror("gpiod_line_event_wait(echo_line)");
            gpiod_line_release(trigger_line);
            gpiod_line_release(echo_line);
            gpiod_chip_close(chip);
            return -1;
        }
        struct gpiod_line_event event;
        rc = gpiod_line_event_read(echo_line, &event);
        if (rc < 0)
        {
            perror("gpiod_line_event_read(gpio_11, &event)");
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
                if (start.tv_sec != 0) // if we didn't miss the start of the pulse
                {
                    finish = event.ts;
                    float pulse_width = ((float)(finish.tv_nsec - start.tv_nsec) / 1000000000) 
                    + (finish.tv_sec - start.tv_sec);
                    float distance = pulse_width * 1100 * 12 / 2.0; // distance in inches based on 1100 fps in air
                    printf("%f, %f\n", pulse_width, distance);
                    start.tv_sec = 0; // zero our for next reading
                    reading_count++;
                }
                need_pulse = true;
            }
            }
        }
    }

    gpiod_line_release(trigger_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
}

struct gpiod_line *init_GPIO(struct gpiod_chip *chip,
                             const char *name,
                             const char *context,
                             line_function func, int flags)
{
    struct gpiod_line *line;

    line = gpiod_chip_find_line(chip, name);
    if (line == NULL)
    {
        return 0;
    }

    if (func == read_line || func == write_line)
    {
        const struct gpiod_line_request_config config =
            {context,
             (func == write_line) ? GPIOD_LINE_REQUEST_DIRECTION_OUTPUT : GPIOD_LINE_REQUEST_DIRECTION_INPUT,
             flags};

        int rc = gpiod_line_request(line,
                                    &config,
                                    0);
        if (0 != rc)
        {
            return 0;
        }
    }
    else if (func == monitor_line)
    {
        // register to read events
        int rc = gpiod_line_request_both_edges_events(line, cnsmr);
        if (0 > rc)
        {
            return 0;
        }
    }
    return line;
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