# Environmental Measurements Spec Delta

## MODIFIED Requirements

### Requirement: Transient measurement handling

The firmware SHALL hold measurements only transiently in memory and emit them through the requested serial console and documented nearby BLE interface. It SHALL NOT persist measurements in files, databases, NVS, or persistent logs, or upload them.

#### Scenario: Normal measurement operation
- **WHEN** measurements are acquired and printed over serial or exposed through the documented BLE interface
- **THEN** firmware measurement handling uses transient memory, console output, and nearby BLE delivery only
- **AND** it does not write measurements to persistent storage or transmit them to a network service
