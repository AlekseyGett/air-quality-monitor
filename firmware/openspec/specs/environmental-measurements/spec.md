# environmental-measurements Specification

## Purpose

Acquire current BME280 environmental measurements at a predictable cadence and present physical units and measurement availability through the serial console.

## Requirements

### Requirement: Configurable BME280 address

The firmware SHALL communicate with the BME280 over I2C at an explicitly configured address of either 0x76 or 0x77, defaulting to 0x76. It SHALL verify BME280 identity before reporting measurements and SHALL NOT substitute a different sensor or silently use another address.

#### Scenario: Sensor at the configured address
- **WHEN** a BME280 responds with the expected identity at the configured address
- **THEN** the firmware initializes that sensor for temperature, humidity, and pressure acquisition

#### Scenario: Missing or incompatible sensor
- **WHEN** no device responds at the configured address or the device identity is not BME280
- **THEN** measurements are reported as unavailable
- **AND** the firmware reports the initialization failure and retries on later acquisition cycles

### Requirement: Five-second acquisition cadence

After successful initialization, the firmware SHALL begin its first measurement without an additional five-second delay. During healthy operation, it SHALL start one fresh measurement of temperature, relative humidity, and atmospheric pressure every five seconds using a schedule that does not accumulate conversion or logging time. Missed deadlines SHALL NOT cause catch-up bursts.

#### Scenario: Healthy periodic acquisition
- **WHEN** the sensor remains available and each acquisition finishes within its deadline
- **THEN** measurement starts are spaced five seconds apart within scheduler resolution
- **AND** one result is printed for each cycle after conversion completes

#### Scenario: Delayed acquisition cycle
- **WHEN** execution misses one or more scheduled acquisition starts
- **THEN** expired starts are skipped and the next future five-second deadline is used
- **AND** the firmware does not issue back-to-back measurements to replay missed cycles

### Requirement: Compensated physical units

The firmware SHALL compensate raw measurements using the sensor's factory calibration and expose temperature in degrees Celsius, relative humidity in percent, and local atmospheric pressure in hPa. Pressure conversion SHALL use 100 Pa per hPa without applying sea-level correction.

#### Scenario: Convert a compensated sample
- **WHEN** compensation produces 23.4 degrees Celsius, 45.6 percent relative humidity, and 101325 Pa
- **THEN** the measurement values are 23.4 degrees Celsius, 45.6 percent, and 1013.25 hPa before presentation rounding

### Requirement: Serial presentation

The firmware SHALL print a timestamped line for each acquisition cycle on the serial console, labeling temperature, humidity, and pressure. Valid temperature and humidity SHALL use one decimal place with units degrees Celsius and percent; valid pressure SHALL use whole hPa rounded to the nearest integer. Invalid fields SHALL be printed as unavailable.

#### Scenario: Present valid measurements
- **WHEN** a sample contains 23.4 degrees Celsius, 45.6 percent humidity, and 1013.25 hPa
- **THEN** a timestamped serial line presents temperature as 23.4 degrees Celsius, humidity as 45.6 percent, and pressure as 1013 hPa with their unit labels

#### Scenario: Present an unavailable field
- **WHEN** humidity is unavailable but temperature and pressure are valid for the current cycle
- **THEN** the serial line prints humidity as unavailable and prints the valid temperature and pressure

### Requirement: Current-cycle validity

The firmware SHALL treat transfer errors, conversion timeouts, invalid raw data, invalid calibration, and non-finite compensated results as unavailable. It SHALL NOT substitute zero or a previous cycle's value. Independently valid fields SHALL remain available where compensation dependencies permit; invalid temperature compensation SHALL invalidate dependent humidity and pressure.

#### Scenario: Failed acquisition after a valid sample
- **WHEN** a cycle fails after the previous cycle produced valid readings
- **THEN** the failed cycle prints unavailable values rather than the previous values or fabricated zeros

#### Scenario: One independent measurement is invalid
- **WHEN** raw humidity is invalid but raw temperature, raw pressure, and their required calibration are valid
- **THEN** humidity is unavailable while current valid temperature and pressure remain available

#### Scenario: Temperature compensation is invalid
- **WHEN** temperature compensation fails
- **THEN** temperature and dependent humidity and pressure are unavailable

### Requirement: Automatic recovery

The firmware SHALL retry initialization or acquisition on subsequent five-second cycles after recoverable sensor failures, without requiring a firmware restart. Retries SHALL be bounded and SHALL NOT reset or recreate the shared bus. Restored valid measurements SHALL automatically replace unavailable output.

#### Scenario: Sensor returns after absence
- **WHEN** an initially absent or failed BME280 resumes valid communication
- **THEN** a subsequent scheduled attempt reinitializes the sensor as needed and reports fresh valid readings
- **AND** no manual firmware restart is required

### Requirement: Transient measurement handling

The firmware SHALL hold measurements only transiently in memory and emit them through the requested serial console. It SHALL NOT persist measurements in files, databases, NVS, or persistent logs, or upload them.

#### Scenario: Normal measurement operation
- **WHEN** measurements are acquired and printed over serial
- **THEN** firmware measurement handling uses transient memory and console output only
- **AND** it does not write measurements to persistent storage or transmit them to a network service
