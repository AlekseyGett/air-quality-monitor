# Verification record

Date: 2026-10-19. Change: `add-bme280-serial-readings`.

## Compilation

`idf.py build` passed using the installed ESP-IDF 6.1 environment and existing
`esp32` target. Final binary size: 0x2cca0 bytes; application partition: 0x100000
bytes (83% free). SDK activation used a local path only in the shell.

The initial sandbox build was blocked by the SDK component manager's `psutil`
process enumeration (`sysctl` permission). Retrying with approved system access
succeeded. SDK configuration emitted pre-existing bool-default and internal
component include warnings during regeneration; project components compiled and
linked successfully. No toolchain check remains blocked.

## Automated behavior

`sh tests/run-host-tests.sh` passed with the real unmodified Bosch C code and
project C++ code, using host transport/lifecycle fakes and a 10 ms fake RTOS clock.
Covered GPIO validation, borrowed bus identity and lifecycle, both addresses,
repeated initialization and cleanup, known calibration/raw compensation and units,
independent invalid channels and calibration, valid zero temperature, non-finite
compensated output rejection, missing/wrong identity, failed transfers, transaction
and conversion timeouts, stuck NVM status, failed-cycle clearing and recovery,
presentation rounding/unavailable fields, processing delay and skipped deadlines.

Ownership mutation checks passed: temporary copies of the sensor with deliberate
bus creation, bus reset, and bus deletion each caused host assertion failure.
Temporary test builds were outside the repository and removed afterward.

## Provenance and change hygiene

All four vendored files were compared byte-for-byte against a checkout of tag
`bme280_v3.5.1`, commit `3ba9abb813660f900365b56e15cac5b7848bb6c4`.
SHA-256 values:

| File | SHA-256 |
| --- | --- |
| bme280.c | 7de1e7d3082b85e90db1daa34573ba77553a7caf44e06e216de5216b7ed4fa12 |
| bme280.h | 6f144510e103f0ab1a24244a71f7ff8ab7f23147b4899aa7bfd0f526062d2163 |
| bme280_defs.h | 541bbd32498b8603bc8fe8eafac26b15d09fb032d37448ca88e7aa398d6a0769 |
| LICENSE | 8c966978a9f67992e824d0b76381eab42f240c0049881f7bf2b46c2c093191aa |

`openspec validate add-bme280-serial-readings --strict` and `git diff --check`
passed. New files were additionally checked with `git diff --no-index --check`
(allowing upstream CRLF line endings); no whitespace diagnostics were found.

Reviewed `sdkconfig`: only the new application configuration block was added,
with GPIOs unset and address 0x76. Target, console, exception/RTTI settings and
root CMake initialization are unchanged. Source review found no measurement
persistence, persistent logging or uploads. The only measurement output is the
requested UART log line. No BLE, Android or shared product document changes were
made. Generated build artifacts remain ignored; no SDK paths were added to project
implementation/configuration. Existing staged work was preserved.

## Hardware

Unverified: no board/module, wiring, electrical setup or serial port was confirmed,
and no flash/monitor operation was attempted. The manual procedure is in
`docs/specs/bme280-acquisition.md`. Physical cadence, accuracy, both hardware
addresses, absence/recovery and seven-day unattended operation were not observed.
Compilation and host tests do not prove these hardware/product acceptance checks.
