// following the example from
// https://github.com/HankB/GPIOD_Debian_Raspberry_Pi/blob/main/60-Capstone_C%2B%2B/follow_input.cpp

/*
Build
g++ -Wall -o hcsr04_distance hcsr04_distance.cpp -lgpiodcxxstd
g++ -Wall -c hcsr04_distance.cpp
*/

#include <iostream>
using namespace std;
#include <gpiod.hpp>
#include <thread>
#include <chrono>

namespace
{
    const ::filesystem::path chip_path("/dev/gpiochip0");
    const ::gpiod::line::offset trigger_line_offset = 24;
    const ::gpiod::line::offset echo_line_offset = 23;
    auto timeout = ::chrono::nanoseconds((long long unsigned)5 * 1000000000); // timeout in nano-seconds
    const char *consumer = "HC-SR04";

    int debug_lvl = 0; // control chattiness
    chrono::_V2::system_clock::duration pulse_send_ts;
    int64_t wait_start_ts;
    int64_t wait_complete_ts;

    gpiod::line::value before_pulse;
    gpiod::line::value during_pulse;
    gpiod::line::value after_pulse;
    gpiod::line::value before_wait;
    gpiod::line::value after_wait;

    int send_pulse(gpiod::line_request &pulse, gpiod::line::offset pulse_offset,
                   gpiod::line_request &echo, gpiod::line::offset echo_offset)
    {
        before_pulse = echo.get_value(pulse_offset);
        pulse.set_value(pulse_offset, ::gpiod::line::value::ACTIVE);
        during_pulse = echo.get_value(pulse_offset);

        // delay 10 microseconds
        this_thread::sleep_for(std::chrono::microseconds(10));
        pulse.set_value(pulse_offset, ::gpiod::line::value::INACTIVE);
        // pulse_send_ts = micros();
        pulse_send_ts = chrono::high_resolution_clock::now().time_since_epoch();
        after_pulse = echo.get_value(pulse_offset);
        ;
        return 0;
    }

} /* namespace */

int main(int argc, char **argv)
{
    cout << "GPIOD version " << gpiod::api_version() << endl;

    // chip object

    gpiod::chip chip("/dev/gpiochip0");
    if (!chip)
    {
        cout << "chip not constructed" << endl;
        exit(-1);
    }
    else
    {
        cout << "chip is constructed and useable" << endl;
    }
    gpiod::chip_info info = chip.get_info();
    cout << "name:" << info.name() << " label:" << info.label() << endl;

    gpiod::edge_event_buffer events;
    cout << "empty buffer holds " << events.num_events() << " events"
         << " and has a capacity of " << events.capacity() << endl;

    // input processing from the GPIOD example follow_input.cpp
    // but probably don;t need the pullup and only interest is
    // the rising input.

    auto echo_request =
        chip.prepare_request()
            .set_consumer(consumer)
            .add_line_settings(
                echo_line_offset,
                ::gpiod::line_settings()
                    .set_direction(
                        ::gpiod::line::direction::INPUT)
                    //                    .set_bias(
                    //                        ::gpiod::line::bias::PULL_UP)
                    .set_edge_detection(
                        ::gpiod::line::edge::RISING))
            .do_request();
    // output processing copied substantially from toggle_line_value.cpp

    auto output_request =
        ::gpiod::chip(chip_path)
            .prepare_request()
            .set_consumer(consumer)
            .add_line_settings(
                trigger_line_offset,
                ::gpiod::line_settings().set_direction(
                    ::gpiod::line::direction::OUTPUT))
            .do_request();

    // initialize the trigger to low
    output_request.set_value(trigger_line_offset, ::gpiod::line::value::INACTIVE);

    if (debug_lvl > 0)
        cerr << "pulse seconds, distance inches" << endl;

    int reading_count = 0;
    bool need_pulse = true;
    while (reading_count < 50)
    {
        if (need_pulse)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(60)); // delay 60 microseconda per recommendation - try 100 ms
            send_pulse(output_request, trigger_line_offset, echo_request, echo_line_offset);

            need_pulse = false;
            if (debug_lvl > 1)
                cout << "\t\t\tpulse rdback " << before_pulse << ", "
                     << during_pulse << ", " << after_pulse << endl;
        }
    }
}