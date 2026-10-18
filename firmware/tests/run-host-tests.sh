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
