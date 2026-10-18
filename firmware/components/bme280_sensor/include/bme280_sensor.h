#pragma once

#include "driver/i2c_master.h"
#include "bme280.h"
#include "measurement.h"

namespace aqm
{
class Bme280
{
public:
    Bme280(i2c_master_bus_handle_t bus, uint8_t address);
    ~Bme280();
    Bme280(const Bme280 &) = delete;
    Bme280 &operator=(const Bme280 &) = delete;
    esp_err_t initialize();
    esp_err_t acquire(Measurement &sample);
    esp_err_t cycle(Measurement &sample);

private:
    static int8_t read(uint8_t reg, uint8_t *data, uint32_t len, void *context);
    static int8_t write(uint8_t reg, const uint8_t *data, uint32_t len, void *context);
    static void delay(uint32_t us, void *context);
    int timeout_ms();
    esp_err_t result(int8_t bosch_result);
    esp_err_t wait_idle(uint8_t mask);
    esp_err_t set_register(uint8_t reg, uint8_t value);
    i2c_master_bus_handle_t bus_;
    i2c_master_dev_handle_t device_ = nullptr;
    uint8_t address_;
    bme280_dev bosch_{};
    esp_err_t last_error_ = ESP_OK;
    int64_t deadline_ = 0;
    bool ready_ = false;
    bool temperature_calibrated_ = false;
    bool pressure_calibrated_ = false;
    bool humidity_calibrated_ = false;
};
}
