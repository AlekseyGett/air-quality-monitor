# Spec Delta

## Purpose

Provide a centrally configured I2C master bus that sensor drivers can share without taking ownership of the bus lifecycle.

## ADDED Requirements

### Requirement: Shared bus ownership

The firmware SHALL initialize the I2C master bus in shared application code and pass the initialized bus handle to the BME280 driver. The bus SHALL remain application-owned and reusable by future sensor drivers. Sensor initialization, acquisition, recovery, and cleanup SHALL NOT create, reset, or delete the shared bus.

#### Scenario: Initialize the BME280 on the shared bus
- **WHEN** application startup initializes the shared bus successfully
- **THEN** it passes that bus handle to the BME280 driver
- **AND** the driver attaches its sensor to that bus without initializing another bus

#### Scenario: Sensor recovery preserves bus ownership
- **WHEN** the BME280 driver retries initialization or releases its sensor resources after a failure
- **THEN** the application-owned bus remains available for other sensor drivers
- **AND** the BME280 driver does not reset or delete the bus

### Requirement: Configurable wiring

The firmware SHALL expose SDA and SCL GPIO selection through build configuration, with both pins unset by default. It SHALL reject unset, identical, or unsupported pins before bus initialization and emit a clear serial diagnostic. Valid configuration SHALL initialize the bus for 100 kHz BME280 communication.

#### Scenario: Explicitly configured wiring
- **WHEN** distinct GPIOs supported for I2C master operation on the ESP32 are configured
- **THEN** startup initializes the bus using the configured SDA and SCL pins
- **AND** BME280 transfers use a 100 kHz clock

#### Scenario: Invalid or missing wiring configuration
- **WHEN** either pin is unset, both pins are identical, or a pin does not support the required operation
- **THEN** startup reports the configuration error over serial
- **AND** sensor acquisition does not start
- **AND** the firmware does not silently select alternative GPIOs

### Requirement: Bounded bus operations

Sensor communication SHALL use finite transaction timeouts. A sensor communication failure SHALL return control to the application so it can report unavailability and retry later without an unbounded wait or busy loop.

#### Scenario: Sensor does not respond
- **WHEN** a BME280 transfer receives no acknowledgement or exceeds its transaction deadline
- **THEN** communication returns a failure within the configured finite timeout
- **AND** the application remains able to schedule subsequent acquisition attempts
