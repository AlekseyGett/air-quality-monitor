#pragma once
#include <cstdint>
using TickType_t = uint32_t;
constexpr int portTICK_PERIOD_MS = 10, pdPASS = 1;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
