#pragma once
#include "esp_err.h"
#include "measurement.h"

namespace aqm
{
// Called once before I2C setup; identity failure leaves BLE disabled.
esp_err_t start_ble();
// Bounded publication from the acquisition task, including unavailable samples.
void publish_measurement(const Measurement &sample);
}
