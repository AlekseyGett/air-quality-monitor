#include "../main/air-quality-monitor.cpp"
#include <cassert>
#include <csetjmp>
#include <cstdio>
#include <initializer_list>
namespace
{
std::jmp_buf exit_loop;
int ble_result, bus_result, ble_starts, sensors, cycles, publications, lines;
int64_t clock_us;
void reset()
{
    ble_result = bus_result = ESP_OK;
    ble_starts = sensors = cycles = publications = lines = 0; clock_us = 0;
}
}
namespace aqm
{
esp_err_t start_ble() { ++ble_starts; return ble_result; }
void publish_measurement(const Measurement &) { ++publications; }
esp_err_t create_i2c_bus(int, int, i2c_master_bus_handle_t *bus)
{
    assert(ble_starts == 1); *bus = nullptr; return bus_result;
}
Bme280::Bme280(i2c_master_bus_handle_t, uint8_t) { ++sensors; }
esp_err_t Bme280::cycle(Measurement &sample)
{
    ++cycles; sample = {}; return cycles == 1 ? ESP_FAIL : ESP_OK;
}
void format_measurement(const Measurement &, char *, size_t) { ++lines; }
int64_t next_deadline(int64_t previous, int64_t now)
{
    assert(previous == (cycles - 1) * 5000000LL && now == previous);
    return previous + 5000000;
}
}
int64_t esp_timer_get_time() { return clock_us; }
const char *esp_err_to_name(int) { return "fake"; }
void fake_log(const char *, const char *, ...) {}
void vTaskDelay(TickType_t ticks)
{
    assert(ticks == 500);
    if (cycles == 2) std::longjmp(exit_loop, 1);
    clock_us += ticks * 10000LL;
}
int main()
{
    reset(); bus_result = ESP_FAIL; app_main();
    assert(ble_starts == 1 && sensors == 0 && publications == 0);
    for (int error : {ESP_OK, ESP_FAIL})
    {
        reset(); ble_result = error;
        if (setjmp(exit_loop) == 0) app_main();
        assert(ble_starts == 1 && sensors == 1 && cycles == 2 && publications == 2 && lines == 2);
    }
    std::puts("Application startup isolation and cadence tests passed");
}
