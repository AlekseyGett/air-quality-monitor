#pragma once

#include <cstddef>
#include <cstdint>

namespace aqm
{
struct Measurement
{
    double temperature = 0;
    double humidity = 0;
    double pressure_hpa = 0;
    bool temperature_valid = false;
    bool humidity_valid = false;
    bool pressure_valid = false;
};

// Defensive finite checks also protect future consumers of compensated values.
void accept_compensated(Measurement &out, double t, double h, double pa,
                        bool t_valid, bool h_valid, bool p_valid);
void format_measurement(const Measurement &sample, char *buffer, size_t size);
// Absolute uptime in microseconds; skip all expired starts, including equality.
int64_t next_deadline(int64_t previous, int64_t now);
}
