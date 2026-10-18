#include "bme280_sensor.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

namespace aqm
{
namespace
{
void sleep_us(uint32_t us)
{
    const uint64_t tick_us = static_cast<uint64_t>(portTICK_PERIOD_MS) * 1000;
    vTaskDelay(static_cast<TickType_t>((us + tick_us - 1) / tick_us));
}
}

Bme280::Bme280(i2c_master_bus_handle_t bus, uint8_t address) : bus_(bus), address_(address)
{
    bosch_.intf = BME280_I2C_INTF;
    bosch_.intf_ptr = this;
    bosch_.read = read;
    bosch_.write = write;
    bosch_.delay_us = delay;
}

Bme280::~Bme280()
{
    if (device_) i2c_master_bus_rm_device(device_);
}

int Bme280::timeout_ms()
{
    if (!deadline_) return 100;
    const int64_t remaining = deadline_ - esp_timer_get_time();
    if (remaining < 1000)
    {
        last_error_ = ESP_ERR_TIMEOUT;
        return 0;
    }
    return static_cast<int>(remaining / 1000);
}

int8_t Bme280::read(uint8_t reg, uint8_t *data, uint32_t len, void *context)
{
    auto &self = *static_cast<Bme280 *>(context);
    int timeout = self.timeout_ms();
    if (!timeout) return -1;
    self.last_error_ = i2c_master_transmit_receive(self.device_, &reg, 1, data, len, timeout);
    return self.last_error_ == ESP_OK ? 0 : -1;
}

int8_t Bme280::write(uint8_t reg, const uint8_t *data, uint32_t len, void *context)
{
    auto &self = *static_cast<Bme280 *>(context);
    uint8_t bytes[32];
    if (len > sizeof(bytes) - 1)
    {
        self.last_error_ = ESP_ERR_INVALID_SIZE;
        return -1;
    }
    bytes[0] = reg;
    std::memcpy(bytes + 1, data, len);
    int timeout = self.timeout_ms();
    if (!timeout) return -1;
    self.last_error_ = i2c_master_transmit(self.device_, bytes, len + 1, timeout);
    return self.last_error_ == ESP_OK ? 0 : -1;
}

void Bme280::delay(uint32_t us, void *context)
{
    auto &self = *static_cast<Bme280 *>(context);
    if (self.deadline_ && esp_timer_get_time() + us >= self.deadline_)
    {
        self.last_error_ = ESP_ERR_TIMEOUT;
        return;
    }
    sleep_us(us);
}

esp_err_t Bme280::result(int8_t code)
{
    if (deadline_ && esp_timer_get_time() >= deadline_) return ESP_ERR_TIMEOUT;
    if (last_error_ != ESP_OK) return last_error_;
    return code == BME280_OK ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t Bme280::set_register(uint8_t reg, uint8_t value)
{
    return result(write(reg, &value, 1, this));
}

esp_err_t Bme280::wait_idle(uint8_t mask)
{
    while (true)
    {
        uint8_t status;
        esp_err_t error = result(read(0xf3, &status, 1, this));
        if (error != ESP_OK) return error;
        if (!(status & mask)) return ESP_OK;
        sleep_us(1000);
    }
}

esp_err_t Bme280::initialize()
{
    ready_ = false;
    last_error_ = ESP_OK;
    if (!bus_ || (address_ != 0x76 && address_ != 0x77)) return ESP_ERR_INVALID_ARG;
    if (!device_)
    {
        i2c_device_config_t config{};
        config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        config.device_address = address_;
        config.scl_speed_hz = 100000;
        esp_err_t error = i2c_master_bus_add_device(bus_, &config, &device_);
        if (error != ESP_OK) return error;
    }
    // Guard Bosch's reset polling and every callback with an elapsed deadline.
    deadline_ = esp_timer_get_time() + 100000;
    bosch_.calib_data = {};
    esp_err_t error = result(bme280_init(&bosch_));
    const auto &c = bosch_.calib_data;
    temperature_calibrated_ = c.dig_t1 != 0 && c.dig_t1 != 0xffff && c.dig_t2 != 0;
    pressure_calibrated_ = c.dig_p1 != 0 && c.dig_p1 != 0xffff;
    humidity_calibrated_ = c.dig_h2 != 0 && c.dig_h2 != -1;
    if (error == ESP_OK && !temperature_calibrated_) error = ESP_ERR_INVALID_RESPONSE;
    if (error == ESP_OK) error = wait_idle(0x09);
    // Reset leaves sleep mode; humidity is latched by the following ctrl_meas write.
    if (error == ESP_OK) error = set_register(0xf2, 0x01);
    if (error == ESP_OK) error = set_register(0xf5, 0x00);
    if (error == ESP_OK) error = set_register(0xf4, 0x24);
    if (error == ESP_OK)
    {
        uint8_t config[4]{};
        error = result(read(0xf2, config, sizeof(config), this));
        if (error == ESP_OK && ((config[0] & 7) != 1 || config[2] != 0x24 || config[3] != 0))
        {
            error = ESP_ERR_INVALID_RESPONSE;
        }
    }
    deadline_ = 0;
    ready_ = error == ESP_OK;
    return error;
}

esp_err_t Bme280::acquire(Measurement &sample)
{
    sample = {};
    if (!ready_) return ESP_ERR_INVALID_STATE;
    last_error_ = ESP_OK;
    deadline_ = esp_timer_get_time() + 100000;
    esp_err_t error = set_register(0xf4, 0x25);
    bme280_settings settings{};
    settings.osr_t = settings.osr_h = settings.osr_p = BME280_OVERSAMPLING_1X;
    uint32_t duration = 0;
    bme280_cal_meas_delay(&duration, &settings);
    if (error == ESP_OK)
    {
        sleep_us(duration);
        error = wait_idle(0x09);
    }
    uint8_t bytes[8]{};
    if (error == ESP_OK) error = result(read(0xf7, bytes, sizeof(bytes), this));
    deadline_ = 0;
    if (error != ESP_OK)
    {
        ready_ = false;
        return error;
    }
    bme280_uncomp_data raw{};
    raw.pressure = (uint32_t(bytes[0]) << 12) | (uint32_t(bytes[1]) << 4) | (bytes[2] >> 4);
    raw.temperature = (uint32_t(bytes[3]) << 12) | (uint32_t(bytes[4]) << 4) | (bytes[5] >> 4);
    raw.humidity = (uint32_t(bytes[6]) << 8) | bytes[7];
    auto &c = bosch_.calib_data;
    bool t_valid = raw.temperature != 0x80000 && temperature_calibrated_;
    bool p_valid = raw.pressure != 0x80000 && pressure_calibrated_;
    bool h_valid = raw.humidity != 0x8000 && humidity_calibrated_;
    bme280_data data{};
    uint8_t mask = BME280_TEMP;
    if (p_valid) mask |= BME280_PRESS;
    if (h_valid) mask |= BME280_HUM;
    if (t_valid)
    {
        error = result(bme280_compensate_data(mask, &raw, &data, &c));
        accept_compensated(sample, data.temperature, data.humidity, data.pressure,
                           error == ESP_OK, h_valid, p_valid);
    }
    if (!sample.temperature_valid || !sample.humidity_valid || !sample.pressure_valid)
    {
        ready_ = false;
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

esp_err_t Bme280::cycle(Measurement &sample)
{
    sample = {};
    esp_err_t error = ready_ ? ESP_OK : initialize();
    return error == ESP_OK ? acquire(sample) : error;
}
}
