#pragma once
#include <cstdint>
#include <cstddef>
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_INVALID_ARG = 1,
              ESP_ERR_INVALID_SIZE = 2, ESP_ERR_TIMEOUT = 3,
              ESP_ERR_INVALID_RESPONSE = 4, ESP_ERR_INVALID_STATE = 5;
using gpio_num_t = int;
struct FakeBus {};
struct FakeDevice {};
using i2c_master_bus_handle_t = FakeBus *;
using i2c_master_dev_handle_t = FakeDevice *;
constexpr int I2C_CLK_SRC_DEFAULT = 0, I2C_ADDR_BIT_LEN_7 = 0;
struct i2c_master_bus_config_t
{
    int i2c_port, sda_io_num, scl_io_num, clk_source, glitch_ignore_cnt;
    struct { bool enable_internal_pullup; } flags;
};
struct i2c_device_config_t
{
    int dev_addr_length, device_address, scl_speed_hz;
};
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *, i2c_master_bus_handle_t *);
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t);
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t, const i2c_device_config_t *, i2c_master_dev_handle_t *);
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t);
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const uint8_t *, size_t, int);
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t, const uint8_t *, size_t, uint8_t *, size_t, int);
