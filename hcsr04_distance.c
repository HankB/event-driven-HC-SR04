/*
Measure distance using an HC-SR04 sensor using Interrupts.

Build:
    gcc -Wall -o hcsr04_distance hcsr04_distance.c  -lwiringPi
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <wiringPi.h>

static void send_pulse(int gpio);
static float read_distance(void);
void handle(void);

static const int echo = 23; // GPIO # for echo input

int main(int argc, char **argv)
{
    float distance;                // distance we measure
    static const int trigger = 24; // GPIO # for trigger output

    // printf("clocks/sec %ld\n", CLOCKS_PER_SEC);
    //  working with clocks/sec 1,000,000 on RpiOS 32 bit, Pi 3B

    // initialize GPIO
    wiringPiSetupGpio();
    pinMode(trigger, OUTPUT);   // configure trigger for output
    digitalWrite(trigger, LOW); // initialize to low

    pinMode(echo, INPUT);                      // configure echo for input
    digitalWrite(echo, LOW);                   // initialize to low
    wiringPiISR(echo, INT_EDGE_BOTH, &handle); // bind ISR

    while (1)
    {
        send_pulse(trigger); // send the trigger pulse
        usleep(500000);      // wait half second (needed?)
        distance = read_distance();
        printf("distance %f\n", distance);
    }
}

static volatile clock_t start_echo; // time echo pulse starts
static volatile clock_t end_echo;   // time return pulse starts

typedef enum
{
    idle = 0,
    waiting_hi = 1,
    wait_lo = 2,
} int_state;

static volatile int_state state = idle;
static volatile int int_count[3];

#define timestamp_count 4
static clock_t timestamp[timestamp_count];

/*
 * At present send_pulse() not only sends the pulse but also polls for the result. for the
 */
static void send_pulse(int gpio)
{
    timestamp[0] = clock();
    state = waiting_hi;
    digitalWrite(gpio, HIGH); // start trigger pulse
    usleep(10);               // 10 microsecond (minimim) sleep
    digitalWrite(gpio, LOW);  // finish trigger pulse
    timestamp[1] = clock();
}

static float read_distance(void)
{
    // 39.5 empirically determined by taking readings at 6", 12" and 18"
    // to convert the readings to inches.
    float distance = ((float)(end_echo - start_echo)) / 39.5;
    printf("%d %d %d ", int_count[0], int_count[1], int_count[2]);
    return distance;
}

void handle(void)
{
    clock_t isr_entry = clock();        // capture time of ISR
    int_count[state]++;                 // count ISR
    int gpio_state = digitalRead(echo); // read the GPIO value

    printf("state%d\n", state);

    switch (state)
    {
    case idle:
        break;

    case waiting_hi:
        if (gpio_state)
        {
            start_echo = isr_entry;
            state = wait_lo;
        }
        break;

    case wait_lo:
        if (!gpio_state)
        {
            end_echo = isr_entry;
            state = idle;
            printf("delta: %ld\n", end_echo - start_echo);
        }
        break;

    default:
        printf("state: %d\n", state);
        exit(-1);
    }
}
