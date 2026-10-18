#include "bme280_sensor.h"
#include "shared_i2c.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

namespace
{
FakeBus bus;
FakeDevice device;
std::array<uint8_t, 256> regs;
int64_t now = 0, busy_until = 0;
int created = 0, added = 0, removed = 0, live = 0, forbidden = 0;
int force_count = 0, reset_count = 0;
bool absent = false, stuck = false, stuck_reset = false, timed_out = false;
bool fail_data = false, fail_add = false;
int expected_address = 0x76;

void put16(int reg, int value)
{
    regs[reg] = value & 255;
    regs[reg + 1] = (value >> 8) & 255;
}

void raw(uint32_t t = 519888, uint32_t p = 415148, uint32_t h = 30000)
{
    regs[0xf7] = p >> 12; regs[0xf8] = p >> 4; regs[0xf9] = p << 4;
    regs[0xfa] = t >> 12; regs[0xfb] = t >> 4; regs[0xfc] = t << 4;
    regs[0xfd] = h >> 8; regs[0xfe] = h;
}

void fixture()
{
    regs.fill(0);
    absent = stuck = stuck_reset = timed_out = fail_data = fail_add = false;
    now = busy_until = 0;
    regs[0xd0] = 0x60;
    // Fixed calibration/raw fixture, evaluated independently with the datasheet
    // appendix 8.1 double formulas (including integer t_fine truncation).
    const int calibration[] = {27504, 26435, -1000, 36477, -10685, 3024,
                              2855, 140, -7, 15500, -14600, 6000};
    for (int i = 0; i < 12; ++i) put16(0x88 + 2 * i, calibration[i]);
    regs[0xa1] = 75;
    put16(0xe1, 362);
    regs[0xe3] = 0;
    // H4=325, H5=50, H6=30.
    regs[0xe4] = 20; regs[0xe5] = 0x25; regs[0xe6] = 3; regs[0xe7] = 30;
    raw();
}

esp_err_t transfer(int timeout)
{
    assert(timeout > 0 && timeout <= 100);
    if (timed_out)
    {
        now += timeout * 1000;
        return ESP_ERR_TIMEOUT;
    }
    return absent ? ESP_FAIL : ESP_OK;
}

void close(double actual, double expected, double tolerance)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::fprintf(stderr, "expected %.9f, got %.9f\n", expected, actual);
        assert(false);
    }
}

void test_pins()
{
    i2c_master_bus_handle_t handle = nullptr;
    for (auto pins : {std::array<int, 2>{-1, 22}, {21, -1}, {21, 21}, {34, 22},
                      {24, 22}, {28, 22}, {40, 22}})
    {
        assert(aqm::create_i2c_bus(pins[0], pins[1], &handle) == ESP_ERR_INVALID_ARG);
    }
    assert(created == 0);
    assert(aqm::create_i2c_bus(21, 22, &handle) == ESP_OK);
    assert(handle == &bus && created == 1);
}

void test_fixture_and_ownership()
{
    fixture();
    const int initial_added = added, initial_removed = removed;
    {
        aqm::Bme280 sensor(&bus, 0x76);
        assert(sensor.initialize() == ESP_OK);
        assert(sensor.initialize() == ESP_OK);
        assert(added == initial_added + 1 && live == 1);
        aqm::Measurement sample;
        const int64_t start = now;
        assert(sensor.acquire(sample) == ESP_OK);
        assert(now - start >= 9300 && now - start <= 100000);
        assert(sample.temperature_valid && sample.humidity_valid && sample.pressure_valid);
        close(sample.temperature, 25.0824779, 0.000001);
        // Published double formula with integer t_fine (128422) as used by Bosch.
        close(sample.pressure_hpa, 1006.532581448, 0.000001);
        // Independent evaluation of datasheet appendix 8.1 humidity expression.
        const double fine = 128422;
        const double x = fine - 76800;
        double humidity = (30000 - (325 * 64 + 50.0 / 16384 * x)) *
                          (362.0 / 65536 * (1 + 30.0 / 67108864 * x));
        humidity *= 1 - 75.0 * humidity / 524288;
        close(sample.humidity, humidity, 0.000001);
        assert(regs[0xf2] == 1 && regs[0xf5] == 0 && (regs[0xf4] & 0xfc) == 0x24);
        assert(force_count > 0 && reset_count > 0);
    }
    assert(live == 0 && removed == initial_removed + 1 && created == 1 && forbidden == 0);
}

void test_failures_and_recovery()
{
    fixture();
    aqm::Bme280 sensor(&bus, 0x76);
    aqm::Measurement sample;
    absent = true;
    assert(sensor.cycle(sample) != ESP_OK && !sample.temperature_valid);
    absent = false;
    assert(sensor.cycle(sample) == ESP_OK && sample.temperature_valid);
    fail_data = true;
    assert(sensor.cycle(sample) != ESP_OK);
    assert(!sample.temperature_valid && !sample.pressure_valid && !sample.humidity_valid);
    fail_data = false;
    assert(sensor.cycle(sample) == ESP_OK);
    stuck = true;
    const int64_t start = now;
    assert(sensor.cycle(sample) == ESP_ERR_TIMEOUT);
    assert(now - start <= 110000 && !sample.temperature_valid);
    stuck = false;
    assert(sensor.cycle(sample) == ESP_OK);
    timed_out = true;
    assert(sensor.cycle(sample) == ESP_ERR_TIMEOUT && !sample.temperature_valid);
    timed_out = false;
    assert(sensor.cycle(sample) == ESP_OK);
    regs[0xd0] = 0x58;
    assert(sensor.initialize() == ESP_ERR_INVALID_RESPONSE);
    regs[0xd0] = 0x60;
    stuck_reset = true;
    const int64_t reset_start = now;
    assert(sensor.initialize() != ESP_OK && now - reset_start <= 110000);
    stuck_reset = false;
    assert(sensor.cycle(sample) == ESP_OK);
}

void test_validity()
{
    fixture();
    aqm::Bme280 sensor(&bus, 0x76);
    aqm::Measurement sample;
    raw(519888, 415148, 0x8000);
    assert(sensor.cycle(sample) != ESP_OK);
    assert(sample.temperature_valid && sample.pressure_valid && !sample.humidity_valid);
    raw(519888, 0x80000);
    assert(sensor.cycle(sample) != ESP_OK);
    assert(sample.temperature_valid && !sample.pressure_valid && sample.humidity_valid);
    raw(0x80000);
    assert(sensor.cycle(sample) != ESP_OK);
    assert(!sample.temperature_valid && !sample.pressure_valid && !sample.humidity_valid);
    raw();
    put16(0x8e, 0);
    assert(sensor.cycle(sample) != ESP_OK && sample.temperature_valid && !sample.pressure_valid);
    put16(0x8e, 36477); put16(0xe1, 0);
    assert(sensor.cycle(sample) != ESP_OK && sample.temperature_valid && !sample.humidity_valid);
    put16(0xe1, 362); put16(0x88, 0xffff);
    assert(sensor.cycle(sample) != ESP_OK && !sample.temperature_valid && !sample.humidity_valid);
    put16(0x88, 27504);
    assert(sensor.cycle(sample) == ESP_OK);
    raw(27504 * 16);
    assert(sensor.cycle(sample) == ESP_OK && sample.temperature_valid);
    close(sample.temperature, 0, 0.000001);
    aqm::accept_compensated(sample, 0, 45.6, 101325, true, true, true);
    assert(sample.temperature_valid && sample.temperature == 0);
    close(sample.pressure_hpa, 1013.25, 0.000001);
    aqm::accept_compensated(sample, 10, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN(), true, true, true);
    assert(sample.temperature_valid && !sample.humidity_valid && !sample.pressure_valid);
    aqm::accept_compensated(sample, std::numeric_limits<double>::quiet_NaN(), 1, 100000, true, true, true);
    assert(!sample.temperature_valid && !sample.humidity_valid && !sample.pressure_valid);
}

void test_format_and_clock()
{
    char line[192];
    aqm::Measurement sample;
    aqm::accept_compensated(sample, 23.4, 45.6, 101325, true, true, true);
    aqm::format_measurement(sample, line, sizeof(line));
    assert(std::strcmp(line, "Temperature: 23.4 °C, Humidity: 45.6 %, Pressure: 1013 hPa") == 0);
    sample.pressure_hpa = 1013.6; sample.humidity_valid = false;
    aqm::format_measurement(sample, line, sizeof(line));
    assert(std::strstr(line, "Humidity: unavailable, Pressure: 1014 hPa"));
    sample = {};
    aqm::format_measurement(sample, line, sizeof(line));
    assert(std::strcmp(line, "Temperature: unavailable, Humidity: unavailable, Pressure: unavailable") == 0);
    int64_t scheduled = 0;
    for (int cycle = 0; cycle < 20; ++cycle)
    {
        scheduled = aqm::next_deadline(scheduled, scheduled + 123456);
        assert(scheduled == (cycle + 1) * 5000000LL);
    }
    assert(aqm::next_deadline(5000000, 16000000) == 20000000);
    assert(aqm::next_deadline(5000000, 10000000) == 15000000);
}
}

int64_t esp_timer_get_time() { return now; }
void vTaskDelay(TickType_t ticks) { assert(ticks > 0); now += ticks * 10000LL; }
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *config, i2c_master_bus_handle_t *out)
{
    assert(!config->flags.enable_internal_pullup);
    ++created; *out = &bus; return ESP_OK;
}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t) { ++forbidden; return ESP_FAIL; }
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t) { ++forbidden; return ESP_FAIL; }
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t handle, const i2c_device_config_t *config, i2c_master_dev_handle_t *out)
{
    assert(handle == &bus && config->scl_speed_hz == 100000);
    assert(config->device_address == expected_address && live == 0);
    if (fail_add) return ESP_FAIL;
    ++added; ++live; *out = &device; return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle)
{
    assert(handle == &device && live == 1); --live; ++removed; return ESP_OK;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t handle, const uint8_t *address, size_t address_len, uint8_t *data, size_t len, int timeout)
{
    assert(handle == &device && address_len == 1 && size_t(*address) + len <= regs.size());
    auto error = transfer(timeout);
    if (error != ESP_OK) return error;
    if (*address == 0xf7 && fail_data) return ESP_FAIL;
    if (*address == 0xf3)
    {
        assert(len == 1);
        *data = stuck_reset ? 1 : ((stuck || now < busy_until) ? 8 : 0);
    }
    else std::memcpy(data, regs.data() + *address, len);
    return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t handle, const uint8_t *bytes, size_t len, int timeout)
{
    assert(handle == &device && len == 2);
    auto error = transfer(timeout);
    if (error != ESP_OK) return error;
    if (bytes[0] == 0xe0) { assert(bytes[1] == 0xb6); ++reset_count; }
    regs[bytes[0]] = bytes[1];
    if (bytes[0] == 0xf4 && (bytes[1] & 3)) { busy_until = now + 9300; ++force_count; }
    return ESP_OK;
}

int main()
{
    test_pins();
    test_fixture_and_ownership();
    test_failures_and_recovery();
    assert(live == 0);
    test_validity();
    test_format_and_clock();
    fixture();
    expected_address = 0x77;
    { aqm::Bme280 sensor(&bus, 0x77); aqm::Measurement s; assert(sensor.cycle(s) == ESP_OK); }
    expected_address = 0x76;
    fixture(); fail_add = true;
    { aqm::Bme280 sensor(&bus, 0x76); assert(sensor.initialize() == ESP_FAIL); }
    { aqm::Bme280 sensor(&bus, 0x75); assert(sensor.initialize() == ESP_ERR_INVALID_ARG); }
    assert(live == 0 && added == removed && created == 1 && forbidden == 0);
    std::puts("PASS: pins, shared bus ownership, fixtures/units, timeouts, validity, recovery, formatting, absolute cadence, both addresses");
}
