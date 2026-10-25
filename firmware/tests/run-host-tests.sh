#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d "${TMPDIR:-/tmp}/aqm-host.XXXXXX")
trap 'rm -rf "$test_build"' EXIT HUP INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -DBME280_DOUBLE_ENABLE \
    -Icomponents/bosch_bme280/include -c components/bosch_bme280/bme280.c -o "$test_build/bosch.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -DBME280_DOUBLE_ENABLE -Itests/fakes -Icomponents/bosch_bme280/include \
    -Icomponents/shared_i2c/include -Icomponents/bme280_sensor/include \
    tests/host_tests.cpp components/shared_i2c/shared_i2c.cpp \
    components/bme280_sensor/bme280_sensor.cpp components/bme280_sensor/measurement.cpp \
    "$test_build/bosch.o" -o "$test_build/host_tests"
"$test_build/host_tests"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -Itests/fakes -Icomponents/device_identity/include \
    tests/identity_tests.cpp components/device_identity/device_identity.cpp -o "$test_build/identity_tests"
"$test_build/identity_tests"
for source in components/nanopb/pb_common.c components/nanopb/pb_encode.c components/ble_measurements/generated/measurements.pb.c; do
    object="$test_build/$(basename "$source").o"
    ${CC:-cc} -std=c99 -Wall -Wextra -Icomponents/nanopb -Icomponents/ble_measurements/generated -c "$source" -o "$object"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -Icomponents/ble_measurements/include -Icomponents/ble_measurements/generated \
    -Icomponents/bme280_sensor/include -Icomponents/nanopb \
    tests/snapshot_tests.cpp components/ble_measurements/measurement_snapshot.cpp \
    "$test_build/pb_common.c.o" "$test_build/pb_encode.c.o" "$test_build/measurements.pb.c.o" \
    -o "$test_build/snapshot_tests"
"$test_build/snapshot_tests" "$test_build/sample.pb"
if [ -n "${PROTO_PYTHON:-}" ]; then
    "$PROTO_PYTHON" tests/check-protobuf.py "$test_build/sample.pb"
    sh scripts/generate-protobuf.sh --check
else
    echo "Independent protobuf decoding/regeneration not run; set PROTO_PYTHON to the pinned tools environment."
fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -Itests/ble_fakes -Itests/fakes -Icomponents/device_identity/include \
    -Icomponents/ble_measurements/include -Icomponents/ble_measurements/generated \
    -Icomponents/bme280_sensor/include -Icomponents/nanopb \
    tests/ble_tests.cpp components/ble_measurements/measurement_snapshot.cpp \
    "$test_build/pb_common.c.o" "$test_build/pb_encode.c.o" "$test_build/measurements.pb.c.o" \
    -o "$test_build/ble_tests"
"$test_build/ble_tests"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -Itests/app_fakes -Itests/ble_fakes -Itests/fakes \
    -Icomponents/ble_measurements/include -Icomponents/shared_i2c/include \
    -Icomponents/bme280_sensor/include tests/app_tests.cpp -o "$test_build/app_tests"
"$test_build/app_tests"
