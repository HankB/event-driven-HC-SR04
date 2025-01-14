/*
Measure distance using an HC-SR04 sensor using event monitoring.

This iteration will eschew the contextless operations which seem to
produce inconsistent results.

Starting point will be some of the code from
https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/blob/main/line_IO/read_write.c

Build:
    gcc -Wall -o hcsr04_distance hcsr04_distance.c  -l gpiod
*/

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <gpiod.h>
#include <stdint.h>

static int send_pulse(struct gpiod_line *);
static void sleep_us(unsigned long microseconds);
static uint64_t micros(void);

typedef enum
{
    read_line,
    write_line,
    monitor_line,
} line_function;

static int debug_lvl = 0; // control chattiness
static uint64_t pulse_send_ts;
static uint64_t wait_start_ts;
static uint64_t wait_complete_ts;

static int before_pulse;
static int during_pulse;
static int after_pulse;
static int before_wait;
static int after_wait;
#define reading_size 300
#define reading_delay 100
static int readings[reading_size];

static struct gpiod_line *trigger_line;
static struct gpiod_line *echo_line;

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
    // struct gpiod_line *trigger_line;
    trigger_line = init_GPIO(chip, trigger_name, cnsmr, write_line, 0);
    if (0 == trigger_line)
    {
        perror("               init_GPIO(trigger_line)");
        gpiod_chip_close(chip);
        return 1;
    }
    // acquire & configure GPIO 23 (echo) for input

    // struct gpiod_line *echo_line;

    echo_line = init_GPIO(chip, echo_name, cnsmr, monitor_line, 0);

    if (0 == echo_line)
    {
        perror("init_GPIO(echo_name)");
        gpiod_line_release(trigger_line);
        gpiod_chip_close(chip);
        return 1;
    }

    if (debug_lvl > 0)
        fprintf(stderr, "pulse seconds, distance inches\n");

    int reading_count = 0;
    bool need_pulse = true;
    while (reading_count < 50)
    {
        if (need_pulse)
        {
            sleep_us(100 * 1000);              // delay 60 microseconda per recommendation - try 100 ms
            int rc = send_pulse(trigger_line); // send the trigger pulse
            if (debug_lvl > 2)
                printf("\t\t\tsend_pulse() rc=%d\n", rc);
            if (0 != rc)
            {
                perror("send_pulse(trigger_line)");
                gpiod_line_release(trigger_line);
                gpiod_line_release(echo_line);
                gpiod_chip_close(chip);
                return -1;
            }
            need_pulse = false;
            if (debug_lvl > 1)
                printf("\t\t\tpulse rdback %d, %d, %d\n", before_pulse, during_pulse, after_pulse);
        }

        const struct timespec timeout = {0L, 1000000L}; // 1000000ns, 0s
        wait_start_ts = micros();
        before_wait = gpiod_line_get_value(echo_line);

        if (debug_lvl > 2)
        {
            for (int i = 0; i < reading_size; i++)
            {
                readings[i] = gpiod_line_get_value(echo_line);
                sleep_us(reading_delay);
            }
        }
        int rc = gpiod_line_event_wait(echo_line, &timeout);
        after_wait = gpiod_line_get_value(echo_line);
        wait_complete_ts = micros();
        if (debug_lvl > 2)
        {
            printf("\t\t\t\t");
            for (int i = 0; i < reading_size; i++)
            {
                printf("%d ", readings[i]);
            }
            printf("\n");
        }
        // uint64_t delta = wait_complete_ts - pulse_send_ts;
        if (debug_lvl > 1)
            printf("\t\t\twait rdback %d, %d\n", before_wait, after_wait);
        if (debug_lvl > 1)
            printf("\t\t\tgpiod_line_event_wait() rc=%d, %lld, %lld\n",
                   rc, wait_start_ts - pulse_send_ts,
                   wait_complete_ts - pulse_send_ts);
        if (rc < 0)
        {
            perror("gpiod_line_event_wait(echo_line)");
            gpiod_line_release(trigger_line);
            gpiod_line_release(echo_line);
            gpiod_chip_close(chip);
            return -1;
        }
        else if (rc == 0)
        {
            need_pulse = true;
        }
        else
        {
            struct gpiod_line_event event;
            rc = gpiod_line_event_read(echo_line, &event);
            if (debug_lvl > 1)
                printf("\t\t\tgpiod_line_event_read() rc=%d, type=%d\n", rc, event.event_type);
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
                        float pulse_width = ((float)(finish.tv_nsec - start.tv_nsec) / 1000000000) + (finish.tv_sec - start.tv_sec);
                        float distance = pulse_width * 1100 * 12 / 2.0; // distance in inches based on 1100 fps in air
                        printf("%f, %f\n", pulse_width, distance);
                        start.tv_sec = 0; // zero our for next reading
                        reading_count++;
                    }
                    need_pulse = true;
                    break;
                }
                default:
                {
                    fprintf(stderr, "\t\t\tUnknown event.event_type %d\n", event.event_type);
                    break;
                }
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
    before_pulse = gpiod_line_get_value(echo_line);
    int rc = gpiod_line_set_value(line, 1);
    if (rc < 0)
    {
        perror("               gpiod_line_set_value(line,1)");
        return 1;
    }
    during_pulse = gpiod_line_get_value(echo_line);

    sleep_us(10); // delay 10 microseconds
    rc = gpiod_line_set_value(line, 0);
    pulse_send_ts = micros();
    after_pulse = gpiod_line_get_value(echo_line);
    if (rc < 0)
    {
        perror("               gpiod_line_get_value(line,0)");
        return 1;
    }
    return 0;
}

// from https://stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
/// Convert seconds to microseconds
#define SEC_TO_US(sec) ((sec) * 1000000)
/// Convert nanoseconds to microseconds
#define NS_TO_US(ns) ((ns) / 1000)

/// Get a time stamp in microseconds.
static uint64_t micros()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    return us;
}

static void sleep_us(unsigned long microseconds)
{
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000ul;           // whole seconds
    ts.tv_nsec = (microseconds % 1000000ul) * 1000; // remainder, in nanoseconds
    nanosleep(&ts, NULL);
}