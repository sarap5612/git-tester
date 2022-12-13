#include "mbed.h"
#include "I2C.h"
#include "stm32f4xx_hal_i2c.h"

#include <chrono>
#include <type_traits>


unsigned long getPressure();

#define I2C_SCL  PA_8
#define I2C_SDA  PC_9  
 

I2C i2c(I2C_SDA, I2C_SCL); // I2C communication

/*
int main() {
    thread_sleep_for(10); // wait for the sensor to boot up
    while (1) {
        unsigned long reading = getPressure();
        printf("Pressure: %d\n", reading);
        thread_sleep_for(1000);
    }
}
*/

unsigned long getPressure() {
    char cmd[3] = {0xAA, 0x00, 0x00}; // default command from datasheet
    char status; // to store the status byte
    char data[4]; // to store the data bytes

    i2c.write(0x30, cmd, 3, true); // send the command
    // i2c.write(0x30);
    // i2c.write(0xAA);
    // i2c.write(0x00);
    // i2c.write(0x00);
    thread_sleep_for(5);
    // request status + 3 data bytes
    // i2c.write(0x31);
    // data[0]=i2c.read(1);
    // data[1]=i2c.read(1);w
    // data[2]=i2c.read(1);
    // data[3]=i2c.read(0);
    i2c.read(0x31, data, 4, false);

    // glue the data together
    unsigned long raw_data = ((long)data[1] << 16) + ((long)data[2] << 8) + ((long)data[3]);

    // do math from datasheet
    unsigned long pressure = raw_data;
    pressure -= 419430;
    pressure *= 300 << 4;
    pressure /= (3774874 - 419430);

    return pressure;
}

using namespace std::literals::chrono_literals;
constexpr auto poll_interval = 1s;//100ms;
constexpr auto breath_time = 5s;
constexpr auto hold_breath_threshold = 10s;
static_assert(breath_time / poll_interval <= 100, "too many threads");


unsigned long global_pressure = 0;
auto read_normalized_pressure() {
    global_pressure = getPressure();
    return global_pressure;
}

template <typename T>
class SchmittTrigger {
    static_assert(std::is_arithmetic<T>::value);

   public:
    const T high_;
    const T low_;
    EventFlags state; /* bit 0 for LOW_STATE, bit 1 for HIGH_STATE */

    explicit SchmittTrigger(T high, T low) : high_(high), low_(low) {}

    bool operator()(T value) noexcept {
        const auto result =
            bool((state.get() & 0b10) ? (value > low_) : (value >= high_));
        state.clear();
        return state.set(1 << result) & 0b10;
    }
};

Ticker fetcher;
Timer checker;

SchmittTrigger<decltype(read_normalized_pressure())> filter(190, 183);
auto oh_no_baby_is_dying() { 0 / 0; }

void check() {
    /* wait for another breath-state to happen */
    const bool prev_state = filter.state.get() & 0b10;
    const auto inv_prev_state = 1 << (!prev_state);

    // FIXME: breath_time? or just hold_breath_threshold?
    const auto result =
        filter.state.wait_all_for(inv_prev_state, hold_breath_threshold, true) &
        inv_prev_state;
    filter.state.set(inv_prev_state);

    if (!result) {
        oh_no_baby_is_dying();
    }
}

//Thread thread[2];

void fetch() {
    auto pressure = read_normalized_pressure();
    auto breath_state = filter(pressure);
    // shouldn't call these in ISR but
    //thread[breath_state].terminate();
    //thread[breath_state].start(check);
    check();
}

void wf() {
    printf("wtf\n");
    0/0;
}

auto previous = false;
auto current = false;
unsigned int prev_edge = 0;

auto is_rising_edge() {
    return ((previous == false) && (current == true));
}

int main() {
    // put your setup code here, to run once:
    thread_sleep_for(10); // wait for the sensor to boot up
    //fetcher.attach(&fetch, poll_interval);
    prev_edge = time(NULL);

    while (1) {
        // put your main code here, to run repeatedly:
        //fetch();
        previous = current;
        current = read_normalized_pressure() > 191;
        printf("pressure: %d\n", global_pressure);
        unsigned int time_now = time(NULL);
        if (is_rising_edge()) {
            printf("rising edge\n");
            prev_edge = time_now;
        }
        if (time_now - prev_edge > 10) {
            goto DEAD;
        }
        thread_sleep_for(100);
    }
    DEAD:
    printf("baby dead, how unfortunate\n");
}