# device-identity Specification

## Purpose

Provide a stable device identity across restarts and expose it to nearby BLE clients through the standard Serial Number characteristic.

## Requirements

### Requirement: Persistent UUIDv4 identity
The firmware SHALL generate a UUIDv4 only when its identity key is absent, store it in non-volatile storage, and successfully commit it before BLE startup. It SHALL reuse a valid stored identity on subsequent boots.

#### Scenario: First boot
- **WHEN** storage is accessible and the identity key is absent
- **THEN** the firmware generates a UUID with version 4 and RFC variant bits and commits it before enabling BLE

#### Scenario: Subsequent boot
- **WHEN** storage contains a valid UUIDv4 identity
- **THEN** the firmware reuses the same identity without generating a replacement

### Requirement: Identity failure isolation
The firmware SHALL prevent BLE startup on storage initialization, identity read, validation, write, or commit failure and report a diagnostic. It SHALL preserve existing storage without automatic erasure or identity replacement, and SHALL allow sensor acquisition and serial output to continue independently.

#### Scenario: Corrupt or inaccessible identity
- **WHEN** identity storage fails or the stored identity is malformed
- **THEN** BLE does not start and a diagnostic is emitted
- **AND** acquisition and serial output remain operational when their own prerequisites are met
- **AND** storage is not erased and no temporary identity is advertised

#### Scenario: First identity commit fails
- **WHEN** generation succeeds but persistence fails
- **THEN** BLE does not start with the uncommitted identity

### Requirement: Standard Serial Number exposure
The firmware SHALL expose the stored UUIDv4 through the read-only Serial Number String characteristic of the standard Device Information Service. Its value SHALL be the lowercase canonical 36-character UUID string without a trailing NUL. Reading SHALL require neither pairing nor authentication.

#### Scenario: Read serial number
- **WHEN** an unpaired compatible client reads Serial Number
- **THEN** it receives the canonical persisted UUID string
- **AND** the value is unchanged after a normal restart
