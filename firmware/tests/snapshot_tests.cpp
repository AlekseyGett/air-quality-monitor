#include "measurement_snapshot.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <initializer_list>
int main(int argc, char **argv)
{
    assert(aqm::notification_fits(20, 23) && !aqm::notification_fits(21, 23));
    assert(aqm::notification_fits(77, 80) && !aqm::notification_fits(78, 80));
    assert(!aqm::notification_fits(0, 22));
    aqm::Measurement sample;
    sample.temperature_valid = sample.humidity_valid = sample.pressure_valid = true;
    sample.temperature = 23.4; sample.humidity = 45.6; sample.pressure_hpa = 1013.25;
    aqm::EncodedMeasurement encoded;
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 13);
    auto save = [&](const char *suffix)
    {
        if (argc != 2) return;
        char path[1024]; std::snprintf(path, sizeof(path), "%s%s", argv[1], suffix);
        FILE *file = std::fopen(path, "wb"); assert(file);
        assert(std::fwrite(encoded.bytes, 1, encoded.size, file) == encoded.size);
        std::fclose(file);
    };
    save("");
    assert(encoded.bytes[10] == 0x18 && encoded.bytes[11] == 0xf5 && encoded.bytes[12] == 0x07);
    sample.pressure_hpa = 1013.5;
    assert(aqm::encode_measurement(sample, encoded) && encoded.bytes[11] == 0xf6);
    save(".rounded");
    sample.temperature = -12.25;
    sample.humidity_valid = false;
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 8);
    save(".partial");
    sample.temperature = 0; sample.pressure_valid = false;
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 5 && encoded.bytes[0] == 0x0d);
    save(".zero");
    for (size_t i = 1; i < 5; ++i) assert(encoded.bytes[i] == 0);
    sample.temperature = std::numeric_limits<double>::infinity();
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 0);
    save(".empty");
    sample.temperature = std::numeric_limits<double>::max();
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 0);
    sample.temperature_valid = false; sample.pressure_valid = true;
    for (double invalid : {-1.0, std::numeric_limits<double>::quiet_NaN(), 2147483647.5})
    {
        sample.pressure_hpa = invalid;
        assert(aqm::encode_measurement(sample, encoded) && encoded.size == 0);
    }
    sample.pressure_hpa = 2147483647.0;
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 6);
    save(".intmax");
    sample.temperature_valid = sample.humidity_valid = true;
    sample.temperature = 0; sample.humidity = 100;
    assert(aqm::encode_measurement(sample, encoded) && encoded.size == 16);
    assert(!aqm::encode_measurement(sample, encoded, 1) && encoded.size == 0);
    for (auto byte : encoded.bytes) assert(byte == 0);
    assert(aqm::encode_measurement(sample, encoded));
    aqm::MeasurementSnapshot snapshot;
    assert(snapshot.read(0).size == 0);
    snapshot.replace(encoded, 100);
    assert(snapshot.read(100).size == encoded.size);
    assert(snapshot.read(15000099).size == encoded.size);
    assert(snapshot.read(15000100).size == 0);
    snapshot.replace(encoded, 15000100); // unchanged sample still refreshes freshness
    assert(snapshot.read(15000100).size == encoded.size);
    snapshot.replace({}, 15000101);
    assert(snapshot.read(15000101).size == 0);
    snapshot.replace(encoded, 15000102);
    assert(snapshot.read(15000102).size == encoded.size);
    std::puts("Snapshot and nanopb tests passed");
}
