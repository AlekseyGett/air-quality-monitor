#pragma once

#include "driver/i2c_master.h"

namespace aqm
{
bool valid_i2c_pins(int sda, int scl);
esp_err_t create_i2c_bus(int sda, int scl, i2c_master_bus_handle_t *bus);
}
