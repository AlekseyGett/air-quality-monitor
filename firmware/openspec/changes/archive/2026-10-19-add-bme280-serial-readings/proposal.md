# Proposal

## Why

The firmware currently has an empty application entry point and cannot acquire or display environmental measurements. Reading the BME280 over a shared I2C bus provides the first independently verifiable measurement increment and a bus that future sensor drivers can reuse.

## What Changes

- Initialize one application-owned I2C master bus with configurable SDA/SCL pins and pass its handle to the BME280 driver.
- Acquire temperature, relative humidity, and atmospheric pressure every five seconds using Bosch SensorAPI.
- Print timestamped serial output in degrees Celsius, percent, and hPa, with explicit unavailable values and automatic recovery after sensor failures.
- Add configuration for the BME280 address, focused automated verification, and setup and hardware-check documentation.
- Keep readings transient in firmware memory; serial output is explicitly requested for this development increment. Do not add product storage, uploads, BLE, other sensors, or Android changes.

## Capabilities

### New Capabilities

- `shared-i2c`: Application-owned I2C initialization, configurable wiring, and bus-handle sharing across sensor drivers.
- `environmental-measurements`: Five-second BME280 acquisition, compensated physical units, validity and recovery semantics, and serial presentation.

### Modified Capabilities

None. No existing OpenSpec capabilities are defined.

## Impact

- Application startup and component registration will initialize and connect reusable shared-I2C and BME280 components.
- New internal interfaces will accept an ESP-IDF bus handle and expose current measurements with independent validity flags.
- Bosch SensorAPI v3.5.1 will be vendored with its license and recorded source revision; ESP-IDF's master I2C component is required.
- New menuconfig options will select GPIOs and the BME280 address without assuming a reference board or module.
- Technical documentation will be added under `docs/specs/`. Existing ESP32 target, root CMake initialization, console settings, and unrelated work are preserved.
