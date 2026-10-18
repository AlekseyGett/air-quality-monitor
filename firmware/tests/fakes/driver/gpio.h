#pragma once
#include <cstdint>
// ESP32 SOC_GPIO_VALID_OUTPUT_GPIO_MASK from soc/soc_caps.h.
#define GPIO_IS_VALID_OUTPUT_GPIO(pin) ((pin) < 34 && ((0x30effffffULL >> (pin)) & 1))
