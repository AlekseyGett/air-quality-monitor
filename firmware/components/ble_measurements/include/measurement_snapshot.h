#pragma once
#include "measurement.h"
#include "measurements.pb.h"
#include <cstddef>
#include <cstdint>

namespace aqm
{
constexpr size_t preferred_payload = 77;
static_assert(Measurements_size <= preferred_payload, "Revise BLE transport budget for schema growth");
constexpr bool notification_fits(size_t size, uint16_t mtu)
{
    return mtu >= 23 && size <= static_cast<size_t>(mtu - 3);
}
struct EncodedMeasurement
{
    uint8_t bytes[Measurements_size] = {};
    size_t size = 0;
};
// Returns false with size zero on encoding failure; never publishes partial bytes.
bool encode_measurement(const Measurement &sample, EncodedMeasurement &out,
                        size_t capacity = Measurements_size);
// Caller synchronizes access. Time is monotonic microseconds.
class MeasurementSnapshot
{
public:
    void replace(const EncodedMeasurement &value, int64_t now);
    EncodedMeasurement read(int64_t now) const;
private:
    EncodedMeasurement value_;
    int64_t updated_ = 0;
    bool present_ = false;
};
}
