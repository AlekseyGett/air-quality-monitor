# Design

## Context

See `proposal.md` for motivation. The application currently has an empty `app_main`, a single registered source, and no sensor components or firmware test harness. ESP-IDF 6.1 targets ESP32; C++ exceptions and RTTI are disabled. The existing UART console is configured for 115200 baud. The compatible SDK is installed locally but must be activated before building; its absolute path must not enter project files.

The shared product requirements establish five-second acquisition, unavailable values on failure, automatic recovery, and no measurement persistence. This increment intentionally prints measurements over serial as requested; it does not establish a BLE interface or satisfy complete product acceptance.

## Goals / Non-Goals

**Goals:** Separate application-owned bus configuration from sensor-specific communication, keep acquisition synchronous and bounded, and expose current-cycle values and independent validity for future consumers.

**Non-Goals:** Introduce a sensor registry, generic bus abstraction, asynchronous acquisition framework, or a second sensor to demonstrate sharing. Do not infer a board, module voltage, wiring, or electrical protection from the ESP32 target.

## Decisions

### Shared ESP-IDF bus with explicit ownership

Add a small `shared_i2c` component that initializes an ESP-IDF master bus and returns its `i2c_master_bus_handle_t`. Application startup owns this handle for the application lifetime and passes it to a separate BME280 component. Use automatic controller selection, default clock source, and the ESP-IDF master bus/device API; register required `esp_driver_i2c` dependencies in component CMake files.

The BME280 component accepts the existing bus handle and configured address, attaches a 7-bit device at 100 kHz, and owns only its device handle and Bosch state. Sensor retries reuse that device handle where possible; cleanup removes only the device. The driver must not reset, recreate, or delete the shared bus. ESP-IDF transaction synchronization is sufficient for this increment; there is one acquisition task, and future sensors will attach their own device handles. No application-wide lock is held while the BME280 converts.

Alternative: initialize I2C inside the sensor driver. Rejected because it couples wiring and bus lifecycle to one sensor. A custom bus framework adds no necessary behavior beyond ESP-IDF's existing handle model.

### Build configuration without assumed GPIOs

Expose `CONFIG_AQM_I2C_SDA_GPIO` and `CONFIG_AQM_I2C_SCL_GPIO` as integer menuconfig options, defaulting to -1 (unset). Validate both as distinct, output-capable ESP32 GPIOs before touching hardware. Board-specific reserved pins remain the user's responsibility and must be described in setup guidance. Invalid configuration or bus creation failure emits an actionable error and ends application startup without a reboot loop or measurement task.

Expose the BME280 address as a menuconfig choice between 0x76 (default) and 0x77. Do not scan or select an alternative address automatically. Disable internal pull-ups and document that suitable external pull-ups to a verified compatible logic voltage are required; a module may already provide them. Do not change the console or ESP32 target configuration. Review sdkconfig changes resulting from adding options, and do not commit machine-specific wiring as a default.

Alternative: hard-code common ESP32 pin assignments or auto-detect the address. Rejected because hardware is unspecified and discovery could conceal incorrect configuration.

### Bosch compensation behind a small adapter

Vendor the source/header files required from Bosch SensorAPI tag `bme280_v3.5.1`, with the upstream license, tag, exact commit, and source URL recorded alongside them. Keep upstream files unchanged and isolated from the project-owned adapter. Compile the API in floating-point compensation mode.

Provide initialization and synchronous sample acquisition operations returning ESP-IDF-style errors. The constructor or explicit initialization inputs are the borrowed bus handle and sensor address. Measurement output contains Celsius, relative humidity percent, and hPa values with independent validity flags, initially all false. Values are usable only when their flags are true; no sample cache or measurement history is introduced. Bosch callback context belongs to the driver instance and remains stable for its lifetime; avoid global sensor state.

Map Bosch register reads to ESP-IDF combined register-address/write and data/read transactions and writes to register-prefixed transmissions. Use a 100 ms timeout per I2C transaction. Map communication errors back to the caller with a useful serial diagnostic. Use FreeRTOS-backed waits rounded upward for required delays rather than busy-waiting through conversions.

Check chip identity, reset/startup completion, factory calibration, and sensor configuration before marking initialization successful. Inspect raw disabled/invalid sentinels before accepting compensated outputs; Bosch compensation can clamp results and must not turn invalid raw inputs into apparently valid measurements. Validate required calibration and reject non-finite compensated values. Invalid humidity or pressure does not discard independently valid fields; invalid temperature invalidates both dependent channels. Convert compensated Pa to hPa with division by 100.0; retain full precision until formatting and apply no altitude correction.

Alternative: implement compensation directly from the datasheet. Bosch's maintained implementation avoids duplicating calibration math and is the user's selected approach.

### One forced conversion per five-second deadline

Use forced mode, x1 oversampling for temperature, humidity, and pressure, and IIR filtering disabled. Forced mode produces one sample on request and then returns to sleep, matching application-controlled five-second acquisition. Normal mode would perform conversions between requested samples without a need in this increment.

Use Bosch's calculated measurement duration, rounded up to the FreeRTOS tick, then check conversion completion with blocking delays and a finite deadline of 100 ms after triggering. Do not wait indefinitely or accept a previous conversion when completion times out. Apply a 100 ms elapsed deadline to reset/startup status polling as well; if an upstream helper hides a polling loop, use bounded adapter logic rather than relying on it indefinitely.

Run acquisition and serial formatting in the existing FreeRTOS application task to avoid an unnecessary worker or queue. Initialize the bus and sensor at startup and trigger the first sample immediately after successful initialization. Anchor periodic scheduling to the first scheduled attempt and use absolute tick deadlines at five-second increments. Advance past elapsed deadlines and block until the next future deadline instead of issuing catch-up acquisitions. Conversion and logging time must not shift the healthy five-second schedule.

On initialization failure, print an all-unavailable cycle and retry initialization at the next five-second deadline. On acquisition failure, mark only justified fields valid, report the error, and attempt sensor initialization again on the next scheduled cycle. A successful retry acquires a new sample in that cycle. Do not retain a previous cycle's readings or use `ESP_ERROR_CHECK` for recoverable sensor errors. Do not reset the shared bus as a sensor recovery action.

### Console output and privacy

Use one `ESP_LOGI` result line per acquisition cycle with the existing uptime timestamp. Format valid fields as `Temperature: 23.4 °C, Humidity: 45.6 %, Pressure: 1013 hPa`; print the literal `unavailable` for invalid fields. Round pressure only for presentation. Separate warning/error diagnostics can explain initialization and acquisition failures, at most once per failed attempt in application code.

Use the UART console only. Do not add log-to-flash, files, NVS measurements, uploads, or history. A user-operated serial capture is external to the firmware. Console measurements are an explicit development requirement, not a reason to change the shared product requirements.

### Focused verification and technical documentation

Add host-runnable tests for project-owned behavior using a fake register transport and real vendored compensation code. Exercise known calibration/raw fixtures with independently established physical expectations, Pa-to-hPa conversion and presentation rounding, invalid channel dependencies, mismatched identity, failed transfers, conversion timeout, retry recovery, and no stale reuse. Verify bus sharing through a fake lifecycle boundary that detects sensor attempts to create/reset/delete a bus. Verify periodic scheduling with a fake clock including a missed deadline. Keep test injection private to the implementation; do not add a generic framework or change upstream Bosch code for tests.

Document configuration, ownership and API contracts, upstream provenance, build commands, external pull-up and voltage prerequisites, and manual checks in `docs/specs/bme280-acquisition.md`. Use the repository's local BME280 datasheet when implementing register/timing details; the upstream references below support the selected approach.

Hardware validation must record actual board/module, GPIOs, address, supply/pull-ups, firmware revision, and serial port. Check at least one minute of five-second cadence, expected units, configured address behavior, absent sensor at boot, and recovery after communication is safely restored. Test both supported addresses with suitable hardware or a fixture. Do not describe physical unplugging as electrically safe without checking the module. Neither host tests nor a build proves hardware performance or seven-day product acceptance.

## Risks / Trade-offs

- Unspecified hardware can compile but cannot acquire readings with unset GPIOs -> require explicit configuration and confirmed electrical details before hardware testing.
- A slave holding the shared bus low may prevent sensor-local recovery -> report bounded failures and retry; global bus recovery affecting future clients is outside this change.
- Bosch compensation can hide raw invalid states through clamping -> validate raw inputs and dependencies before declaring fields valid.
- A missed deadline reduces observed sample count -> skip expired deadlines, preserve the normal cadence, and verify the delayed path without bursts.
- Vendored dependencies require deliberate maintenance -> retain upstream code and license, record an immutable revision, and test the adapter against fixtures.

## Migration Plan

No data migration is required. During implementation, add components, menuconfig options, documentation, and tests, then build using ESP-IDF 6.1 without changing the target. Configure actual pins/address before flashing a user-specified board and port. Rollback uses the previous firmware revision; no stored measurements require cleanup. Existing unrelated staged changes remain untouched.

## References

- [ESP-IDF I2C master bus/device API](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/i2c.html)
- [Bosch SensorAPI v3.5.1](https://github.com/boschsensortec/BME280_SensorAPI/releases/tag/bme280_v3.5.1)
- Local datasheet: `docs/datasheets/bme280.pdf`; [Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)
