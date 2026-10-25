# Tasks

## 1. Persistent identity

- [x] 1.1 Add an identity component using the dedicated NVS namespace/key, UUIDv4 validation and formatting, and ESP-IDF 6.1 entropy-backed generation before RF startup; verify generation/entropy lifecycle against the installed SDK and host-test UUID version, variant, canonical formatting, and reuse.
- [x] 1.2 Add storage boundary fakes and failure tests for absent key, malformed length/version/variant, initialization/read/write/commit errors, and reboot reuse; verify only absence generates identity and errors never erase storage or expose an uncommitted UUID using `sh tests/run-host-tests.sh`.
- [x] 1.3 Document identity lifetime, full-erase behavior, and failure diagnostics in `docs/specs/ble-interface.md`; verify the documented behavior matches the identity tests.

## 2. Measurement snapshots and protobuf

- [x] 2.1 Vendor a pinned nanopb runtime and license, pin the matching generator and compatible protoc/Python dependencies, and check in bindings generated directly from `../protocol/measurements.proto`; add an explicit reproducible regeneration command and temporary-directory consistency check, verifying ordinary firmware/host builds need neither generation nor dependency downloads.
- [x] 2.2 Add bounded nanopb serialization from `Measurement` with generated optional presence flags, finite float conversion and whole integer hPa rounding directly from internal pressure; verify independent protobuf decoding for full/partial/empty messages, valid zero, negative temperature, non-finite values, conversion boundaries, schema size within the 77-byte preferred payload budget, actual default-MTU payload size, and encoding failure/buffer exhaustion producing an empty snapshot without partial or stale output.
- [x] 2.3 Add the coherent transient snapshot boundary and 15-second read expiry with injectable time; verify initial empty reads, replacement of absent fields, exact expiry boundary, failure/recovery, and unchanged-value publication using focused host tests.
- [x] 2.4 Document nanopb runtime/generator pins, generation prerequisites, regeneration and consistency-check commands, the schema link, units, pressure rounding, complete-snapshot semantics, empty payloads, receipt-time freshness limitations, and representative payloads in `docs/specs/ble-interface.md`; verify examples with the independent protobuf decoder.

## 3. NimBLE services and lifecycle

- [x] 3.1 Register the standard Device Information Service and runtime Serial Number plus the fixed custom service with exactly one read/notify characteristic and CCCD; register component dependencies, configure preferred ATT MTU 80, and review necessary sdkconfig changes, verifying with an ESP-IDF 6.1 build and focused service-definition checks.
- [x] 3.2 Add connectable advertising with custom service UUID and name in scan response, one-client subscription lifecycle, bounded advertising restart, and host resynchronization handling; verify disconnect/failure event transitions with fakes and document discovery/reconnect behavior.
- [x] 3.3 Add coherent GATT reads, correct Serial Number offset handling, and bounded host-context notification publication including zero-length messages; verify subscription changes, stale/disconnected handles, allocation/send failures, and latest-value coalescing without backlog using transport-boundary tests; track negotiated MTU, verify exact payload-capacity boundaries at MTU 23 and 80, and skip oversized notifications without truncation, fragmentation, or blocking acquisition.
- [x] 3.4 Complete the public GATT table and independent-client verification procedure in `docs/specs/ble-interface.md`, including preferred MTU 80/client exchange, the 77-byte payload budget, smaller-MTU fallback, oversized-notification behavior, default-MTU identity reads, and empty notifications; verify every identifier and property matches service definitions and the shared schema.

## 4. Application integration and system verification

- [x] 4.1 Integrate independent identity/BLE startup before I2C early returns and publish after every acquisition cycle without changing absolute scheduling or serial behavior; verify fault injection permits acquisition on identity/BLE failure and BLE empty reads on sensor startup failure.
- [x] 4.2 Run `sh tests/run-host-tests.sh`, an ESP-IDF 6.1 `idf.py build`, `git diff --check`, and OpenSpec validation; review all configuration diffs and record results or unavailable-toolchain limitations separately from hardware evidence.
- [x] 4.3 Document a hardware acceptance checklist covering unpaired discovery/read/notify, UUID persistence across power cycles, partial/complete sensor failure and recovery, unchanged readings, 15-second expiry, reconnect, negotiated MTU 80, default-MTU fallback, oversized notification rejection, default-MTU reads, and seven-day measurement/BLE availability with daily reconnects; verify procedure coverage against the specs and explicitly record which checks were actually run. Hardware execution requires an explicitly selected board/port and user request.
