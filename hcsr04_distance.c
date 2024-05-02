/*
Measure distance using an HC-SR04 sensor using Interrupts.

Build:
    gcc -Wall -o hcsr04_distance hcsr04_distance.c  -lwiringPi
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>

static void send_pulse(int gpio);
static float read_distance(void);

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
    pinMode(echo, INPUT);       // configure trigger for output
    digitalWrite(echo, LOW);    // initialize to low

    while (1)
    {

        send_pulse(trigger); // send the trigger pulse
        usleep(500000);      // wait half second (needed?)
        distance = read_distance();
        printf("distance %f\n", distance);
    }
}

static clock_t start_echo; // time echo pulse starts
static clock_t end_echo; // time return pulse starts

static void send_pulse(int gpio)
{
#define timestamp_count 5
    static clock_t timestamp[timestamp_count];

    timestamp[0] = clock();
    digitalWrite(gpio, HIGH);            // start trigger pulse
    usleep(10);                          // 10 microsecond (minimim) sleep
    digitalWrite(gpio, LOW);             // finish trigger pulse
    timestamp[1] = clock();

    // first cut, poll for transions Low to high indicates
    while (!digitalRead(echo))
    {
        usleep(1); // 1 microsecond (minimim) sleep
    }
    timestamp[2] = start_echo = clock(); // mark start of pulse
    timestamp[3] = clock();

    while (digitalRead(echo))
    {
        usleep(1); // 1 microsecond (minimim) sleep
    }
    timestamp[4] = end_echo = clock(); // mark end of echo

    for (int i = 1; i < timestamp_count; i++)
    {
        printf("%ld, ", timestamp[i] - timestamp[0]);
    }
    printf("\n");
}

static float read_distance(void)
{
    // 39.5 empirically determined by taking readings at 6", 12" and 18"
    // to convert the readings to inches.
    float distance = ((float)(end_echo - start_echo)) / 39.5; 
    return distance;
}
