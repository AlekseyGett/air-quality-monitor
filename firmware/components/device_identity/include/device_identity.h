#pragma once
#include "esp_err.h"

namespace aqm
{
// Call once, before RF or ADC initialization. Output is empty on every failure.
esp_err_t load_device_identity(char (&serial)[37]);
}
