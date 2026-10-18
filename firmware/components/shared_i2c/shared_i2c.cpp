#include "shared_i2c.h"
#include "driver/gpio.h"

namespace aqm
{
bool valid_i2c_pins(int sda, int scl)
{
    return sda >= 0 && scl >= 0 && sda != scl &&
           GPIO_IS_VALID_OUTPUT_GPIO(sda) && GPIO_IS_VALID_OUTPUT_GPIO(scl);
}

esp_err_t create_i2c_bus(int sda, int scl, i2c_master_bus_handle_t *bus)
{
    if (!bus || !valid_i2c_pins(sda, scl))
    {
        return ESP_ERR_INVALID_ARG;
    }
    *bus = nullptr;
    i2c_master_bus_config_t config{};
    config.i2c_port = -1;
    config.sda_io_num = static_cast<gpio_num_t>(sda);
    config.scl_io_num = static_cast<gpio_num_t>(scl);
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = false;
    return i2c_new_master_bus(&config, bus);
}
}
