#include "measurement_snapshot.h"
#include "pb_encode.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace aqm
{
bool encode_measurement(const Measurement &sample, EncodedMeasurement &out, size_t capacity)
{
    out = {};
    Measurements message = Measurements_init_zero;
    const double max_float = std::numeric_limits<float>::max();
    message.has_temperature = sample.temperature_valid && std::isfinite(sample.temperature) &&
                              std::abs(sample.temperature) <= max_float;
    message.has_humidity = sample.humidity_valid && std::isfinite(sample.humidity) &&
                           std::abs(sample.humidity) <= max_float;
    if (message.has_temperature) message.temperature = static_cast<float>(sample.temperature);
    if (message.has_humidity) message.humidity = static_cast<float>(sample.humidity);
    if (sample.pressure_valid && std::isfinite(sample.pressure_hpa) && sample.pressure_hpa >= 0)
    {
        const double rounded = std::round(sample.pressure_hpa);
        message.has_pressure = rounded <= std::numeric_limits<int32_t>::max();
        if (message.has_pressure) message.pressure = static_cast<int32_t>(rounded);
    }
    pb_ostream_t stream = pb_ostream_from_buffer(out.bytes, std::min(capacity, sizeof(out.bytes)));
    if (!pb_encode(&stream, Measurements_fields, &message))
    {
        out = {};
        return false;
    }
    out.size = stream.bytes_written;
    return true;
}
void MeasurementSnapshot::replace(const EncodedMeasurement &value, int64_t now)
{
    value_ = value;
    updated_ = now;
    present_ = true;
}
EncodedMeasurement MeasurementSnapshot::read(int64_t now) const
{
    if (!present_ || now < updated_ || now - updated_ >= 15000000) return {};
    return value_;
}
}
