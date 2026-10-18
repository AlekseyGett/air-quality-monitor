#include "measurement.h"
#include <cmath>
#include <cstdio>

namespace aqm
{
void accept_compensated(Measurement &out, double t, double h, double pa,
                        bool t_valid, bool h_valid, bool p_valid)
{
    out = {};
    out.temperature_valid = t_valid && std::isfinite(t);
    out.humidity_valid = out.temperature_valid && h_valid && std::isfinite(h);
    out.pressure_valid = out.temperature_valid && p_valid && std::isfinite(pa);
    if (out.temperature_valid) out.temperature = t;
    if (out.humidity_valid) out.humidity = h;
    if (out.pressure_valid) out.pressure_hpa = pa / 100.0;
}

void format_measurement(const Measurement &sample, char *buffer, size_t size)
{
    char t[40] = "unavailable", h[40] = "unavailable", p[40] = "unavailable";
    if (sample.temperature_valid) std::snprintf(t, sizeof(t), "%.1f °C", sample.temperature);
    if (sample.humidity_valid) std::snprintf(h, sizeof(h), "%.1f %%", sample.humidity);
    if (sample.pressure_valid) std::snprintf(p, sizeof(p), "%.0f hPa", sample.pressure_hpa);
    std::snprintf(buffer, size, "Temperature: %s, Humidity: %s, Pressure: %s", t, h, p);
}

int64_t next_deadline(int64_t previous, int64_t now)
{
    constexpr int64_t period = 5000000;
    return previous + ((now >= previous ? (now - previous) / period : 0) + 1) * period;
}
}
