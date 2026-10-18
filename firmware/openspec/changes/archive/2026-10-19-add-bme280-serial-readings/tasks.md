# Tasks

## 1. Shared I2C bus and configuration

- [x] 1.1 Add the shared I2C component and application-owned bus creation using the ESP-IDF 6.1 master API, register component dependencies, and verify the component compiles without changing the ESP32 target or root CMake initialization order.
- [x] 1.2 Add menuconfig SDA/SCL options defaulting to unset and BME280 address selection defaulting to 0x76; validate unset, identical, and unsupported GPIOs before initialization, and verify valid/invalid configurations with focused host tests and review the sdkconfig diff.
- [x] 1.3 Establish a minimal host test command with fake ESP-IDF transport/lifecycle boundaries; verify a borrowed bus can be passed to a sensor device and that the tests detect unintended bus creation/reset/deletion by a sensor.
- [x] 1.4 Start `docs/specs/bme280-acquisition.md` with bus ownership, configuration steps, 100 kHz operation, external pull-up and logic-voltage prerequisites; verify option names match the implementation and avoid assumed board-specific GPIOs.

## 2. BME280 driver and physical measurements

- [x] 2.1 Vendor the required Bosch SensorAPI v3.5.1 files and license as an isolated dependency, record the exact upstream commit and URL, and verify file provenance and compilation in floating-point mode.
- [x] 2.2 Implement the BME280 adapter accepting the borrowed bus handle and configured address, owning only device/Bosch state, with 100 ms transaction timeouts and bounded initialization; verify identity mismatch, missing device, timeout, and repeated initialization without leaked device handles through fake transport tests.
- [x] 2.3 Implement forced acquisition with x1 oversampling for all channels, filter disabled, Bosch-derived wait timing, and a bounded completion check; verify a known raw/calibration fixture produces expected physical values and a stuck conversion returns an error without blocking indefinitely.
- [x] 2.4 Return Celsius, relative humidity percent, and pressure divided by 100.0 into hPa with independent validity flags; verify raw sentinels, invalid calibration, non-finite results, partial validity, temperature dependency failures, valid zero temperature, and failed-cycle clearing with focused tests.
- [x] 2.5 Extend the technical documentation with the driver contract, calibration/validity behavior, forced-mode timing, address configuration, and vendored dependency provenance; verify references against the local BME280 datasheet and selected Bosch revision.

## 3. Periodic acquisition and serial presentation

- [x] 3.1 Connect shared bus initialization and BME280 acquisition from `app_main`, taking the first sample after initialization and using absolute five-second deadlines; verify fake-clock tests cover processing delay and skipped deadlines without cadence drift or catch-up bursts.
- [x] 3.2 Add one timestamped serial result per cycle with one-decimal Celsius/humidity, rounded integer hPa, and explicit unavailable fields; verify representative samples, rounding, and partial/all-unavailable output in host tests.
- [x] 3.3 Retry sensor initialization/acquisition on subsequent five-second cycles after failures while preserving the bus; verify absent-at-startup and valid-to-failed-to-recovered sequences report only fresh current-cycle values and require no firmware restart.
- [x] 3.4 Complete setup/build/monitor documentation and a hardware verification procedure covering at least one minute of cadence, units, both addresses, absent sensor, and restored communication; verify commands use an explicit port placeholder and require confirmed board/module, wiring, voltage, and pull-ups before hardware operations.

## 4. Integration verification

- [x] 4.1 Run all focused host tests and `idf.py build` in a compatible ESP-IDF 6.1 environment; record compilation and automated-test outcomes separately and report any toolchain limitation explicitly.
- [x] 4.2 Run `git diff --check`, review configuration and storage/logging paths, and confirm the resulting change adds no measurement persistence, uploads, BLE, Android changes, generated outputs, or machine-specific SDK paths; record hardware checks as unverified unless actually performed on confirmed hardware.
