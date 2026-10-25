# Proposal

## Why

The device currently exposes BME280 measurements only over serial. A documented BLE interface is needed for Android and other nearby clients to identify the device and receive current measurements without pairing or internet access.

## What Changes

- Add a standard Device Information Service exposing a persistent UUIDv4 through Serial Number.
- Generate identity only when its NVS key is absent and commit it before BLE starts; identity-storage failures block BLE startup while measurement acquisition continues.
- Add a custom Air Quality Measurements Service with exactly one read/notify characteristic, using the existing `../protocol/measurements.proto` message.
- Encode temperature in degrees Celsius, humidity in percent, and pressure in whole integer hPa; omit unavailable fields and publish each acquisition cycle.
- Configure preferred ATT MTU to 80 bytes and send each complete message in one notification within the negotiated payload capacity, without application fragmentation.
- Document discovery, fixed service/characteristic UUIDs, encoding, freshness, reconnect behavior, and hardware verification.

## Capabilities

### New Capabilities

- `device-identity`: Durable UUIDv4 identity, storage failure behavior, and standard BLE Serial Number exposure.
- `ble-measurements`: Discoverable, unauthenticated BLE access to current protobuf measurement snapshots.

### Modified Capabilities

- `environmental-measurements`: Permit transient measurement delivery through the documented BLE interface in addition to serial, preserving the prohibition on persistence and uploads.

## Impact

Firmware startup, new identity and BLE components, component registrations, and intentional NimBLE configuration changes are affected. Add nanopb runtime and generated message bindings, pinned generation dependencies, bounded protobuf serialization, and focused host tests, plus `docs/specs/ble-interface.md`. The shared protobuf schema remains unchanged; Android implementation and shared product documents are outside scope. Only identity is persisted; measurements remain transient. Compilation, hardware BLE checks, and seven-day acceptance remain distinct verification activities.
