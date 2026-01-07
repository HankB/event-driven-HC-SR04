// play with microsecond timing in c++
#include <chrono>
#include <iostream>
#include <thread>
#include <time.h>

/*
g++ -o microseconds -Wall microseconds.cpp
1767721698 250 684
*/

// Source - https://stackoverflow.com/a
// Posted by Lorien Brune, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-06, License - CC BY-SA 4.0

using namespace std::chrono;
using namespace std;

static void sleep_us(unsigned long microseconds)
{
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000ul;           // whole seconds
    ts.tv_nsec = (microseconds % 1000000ul) * 1000; // remainder, in nanoseconds
    nanosleep(&ts, NULL);
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

int main()
{
    auto start_ts = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    auto start_us = micros();
    this_thread::sleep_for(std::chrono::microseconds(10));

    cout << start_ts << endl;
    start_ts = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    start_us = micros();
    this_thread::sleep_for(std::chrono::microseconds(10));
    auto end_us = micros();
    auto end_ts = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    cout << end_ts - start_ts << endl;
    cout << end_us - start_us << endl;

    start_ts = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    sleep_us(10);
    end_ts = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    cout << end_ts - start_ts << endl;

    return 0;
}
