#pragma once
#include "measurement.h"
#include "driver/i2c_master.h"
namespace aqm
{
class Bme280
{
public:
    Bme280(i2c_master_bus_handle_t, uint8_t);
    esp_err_t cycle(Measurement &);
};
}
