# Design

## Context

See proposal.md for motivation and the capability deltas for the public contract. The current `app_main` owns the I2C bus and runs an absolute five-second sensor loop. `Measurement` carries per-field validity and pressure as unrounded double hPa. NimBLE peripheral and GATT server support are already enabled, but no BLE application or identity storage exists. The shared proto3 schema has optional float temperature/humidity and optional int32 pressure; it has no timestamps, sequence numbers, or envelope.

## Goals / Non-Goals

**Goals:** Isolate identity/storage and BLE lifecycle from sensor recovery, keep acquisition scheduling bounded, and provide a complete interoperable snapshot with preferred ATT MTU 80 and compatibility with clients retaining the default MTU.

**Non-Goals:** Android implementation, schema changes, pairing, measurement history, guaranteed notification delivery, additional measurement characteristics, or concurrent-client guarantees. Standard GAP/GATT infrastructure is permitted alongside the two requested application services.

## Decisions

### 1. Dedicated persistent identity component

Use the existing default NVS partition with a dedicated namespace `aqm_identity` and blob key `uuid` containing 16 UUID bytes. Read and validate length, version, and variant before use. Generate only on an explicitly absent key. Use ESP-IDF hardware randomness with an active documented entropy source, set UUIDv4 and RFC variant bits, write, and commit before starting BLE. Temporarily enable the supported internal entropy source before BLE/controller initialization, then disable it before RF initialization; verify the exact ESP-IDF 6.1 API and required component dependency during implementation. Do not use timestamps, MAC-derived IDs, or an unseeded software PRNG.

Any initialization/read/validation/write/commit error returns a recoverable startup status to the application: log the failure, skip BLE for this boot, and continue the sensor path. Do not use an aborting error macro, automatically erase NVS, replace corrupt identity, or advertise a temporary ID. A later restart can retry. Normal firmware updates preserve the partition; an intentional full flash erase loses identity and causes generation on the next boot. This avoids silently changing an established identity as a recovery side effect.

### 2. NimBLE services and discovery

Use NimBLE already selected in sdkconfig. Register standard Device Information Service `0x180A` with read-only Serial Number String `0x2A25`, serving the canonical lowercase UUID without its C terminator. Use the built-in DIS implementation if its ESP-IDF 6.1 serial-number configuration supports the required runtime value; otherwise register an equivalent standard service, never both. Do not enable extra DIS fields without need.

Register the fixed custom service and characteristic UUIDs from the BLE spec with read/notify properties and the standard CCCD. Keep the characteristic value handle separate from the CCCD handle. Use no authentication/encryption access flags. Standard GAP/GATT services are infrastructure, not additional custom measurements services.

Advertise the complete custom service UUID in connectable legacy advertising, and put the proposed name `Air Quality Monitor` in the scan response to stay within advertising limits. The service UUID is the compatibility filter; Serial Number provides durable identity after connection. Support one client as the MVP default and clear subscription state on disconnect. Resume advertising after disconnect, failed connection, and host resynchronization; keep restart attempts bounded and out of a busy loop. Review only necessary sdkconfig changes, including runtime Serial Number support if used.

Set `CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU` to 80 through intentional ESP-IDF configuration, reviewing any generated compatibility alias as part of the sdkconfig diff. The current tracked preference is 256; this change selects the explicit 80-byte contract. MTU exchange is client-initiated and the effective MTU depends on both peers. Document that clients should request an MTU of at least 80; retain default-MTU compatibility for current samples before or without exchange.

Each notification carries exactly one complete protobuf message. Check its encoded length against the connection's negotiated ATT MTU minus three before sending. Never truncate or split a message, and never add application segmentation/reassembly. If it exceeds capacity, skip that notification with a bounded diagnostic, retain the complete readable snapshot, and continue acquisition. Reset tracked MTU state on disconnect and update it on MTU exchange. Future schema additions must keep the generated worst-case bound within the 77-byte preferred payload budget or deliberately revise this transport contract; preference 80 alone cannot guarantee arbitrary future messages or peer support. Lower-layer BLE fragmentation remains the stack's responsibility.

### 3. Bounded nanopb serialization

Use nanopb-generated C bindings from the authoritative `../protocol/measurements.proto` and the nanopb encoding runtime. Pin the runtime and generator to the same release and pin compatible protoc/Python generation dependencies during implementation. Vendor the required runtime sources and license in a firmware component and check generated `measurements.pb.c`/`.pb.h` into the firmware serialization component. Ordinary firmware and host builds compile these sources without running a generator or downloading dependencies. Provide a documented explicit regeneration command and a consistency check that regenerates into a temporary directory and compares outputs. Generation must use reproducible options without timestamps or machine-specific paths; do not maintain a second schema copy or hand-edit generated bindings.

Populate the generated message from `Measurement`, setting its optional presence flags explicitly, including for valid zero values. Use `pb_encode` with `pb_ostream_from_buffer` into a fixed-capacity buffer, without dynamic allocation or custom field encoders. Guard finite float conversion and integer bounds. Round internal hPa directly to the nearest whole hPa, with positive half values rounded upward, and encode as int32; do not multiply by 100. For example, 1013.25 hPa becomes 1013 and 1013.5 hPa becomes 1014. Invalid fields are omitted. Keep the shared schema unchanged; any future nanopb-specific options belong in firmware-local generation configuration.

Size the buffer using the generated maximum encoded-size constant, and verify it against the preferred 77-byte notification payload budget (ATT MTU 80 minus three bytes of ATT overhead). The current schema needs at most 21 bytes for unrestricted int32 values because negative int32 uses a ten-byte varint; valid nonnegative pressure limits actual application payloads to at most 16 bytes. Negative or physically invalid pressure is unavailable under sensor validity rules. Current valid samples therefore also fit the default 20-byte notification payload. Test both the schema bound against the preferred budget and actual application payloads against the default-MTU budget. No framing, length prefix, checksum, or custom fragmentation is needed.

Check encoding success before publishing bytes. On encoding failure, report a diagnostic and publish an empty unavailable snapshot; never expose partial output or retain an older valid measurement as current. Decode fixtures using an independent protobuf implementation against the shared schema, and test presence, units, buffer exhaustion, and regeneration consistency. Nanopb replaces the previously proposed handwritten encoder so field encoding stays generated from the shared contract.

### 4. Atomic transient snapshot and host-context notifications

Keep one bounded encoded snapshot and its cycle-completion uptime in RAM, initially empty. After every cycle, including failures and unchanged values, serialize a fresh sample and atomically replace the snapshot. Use a short FreeRTOS lock around copying bytes/timestamps only; never hold it during sensor I/O or BLE operations. GATT reads copy a coherent snapshot and return empty when no cycle exists or its age is at least 15 seconds. Read offsets must work for the 36-byte Serial Number at a default MTU.

Signal publication to the NimBLE host context with a bounded/coalescing event. The host sends the latest snapshot to the currently subscribed connection using an explicit payload; it never drains a measurement history. Verify connection/subscription state in host context so queued events cannot target a disconnected handle. Log/rate-limit recoverable allocation or notification errors and retain the current readable snapshot. A slow or disconnected client cannot delay the sensor loop. Deliver the zero-length notification for the valid empty protobuf message; verify this explicitly on hardware.

On subscription the client can read the latest fresh snapshot or await the next cycle. No historical notifications are replayed. Document that messages replace all fields, absent fields clear previous values, and clients track a 15-second timeout for each present field. A reconnect starts a new client freshness window; a read can return the device's current snapshot if younger than 15 seconds. The schema conveys no original sample age, so clients cannot infer a device timestamp from receipt time.

### 5. Independent startup paths

Initialize identity and start BLE through a separate startup path before the existing I2C setup can return on invalid wiring. BLE can therefore advertise and return empty snapshots even if sensor startup fails. Conversely identity/BLE failure cannot prevent the existing acquisition/serial loop from running. Preserve application-owned bus lifecycle, automatic sensor recovery, and absolute cycle scheduling. If BLE startup fails after partial initialization, clean up owned BLE resources without rebooting or altering the shared bus.

## Risks / Trade-offs

- Generated bindings can become stale or mismatch the nanopb runtime → pin matching runtime/generator releases, check regeneration consistency, and independently decode fixtures.
- A peer can retain a smaller MTU and future messages can outgrow it → check negotiated capacity, skip oversized notifications without truncation, and document client MTU exchange and the 77-byte evolution budget.
- Notifications are unacknowledged and resources can be exhausted → bounded latest-value handling, readable fallback, and documented client freshness timeout.
- The protobuf has no timestamp → never serve firmware-stale snapshots; document receipt-time freshness and do not invent envelope fields.
- NVS corruption or exhausted storage makes BLE unavailable → explicit diagnostics and deliberate operator recovery; no automatic erase that destroys identity.
- Boot-time randomness depends on hardware entropy prerequisites → verify the ESP32/ESP-IDF 6.1 entropy lifecycle before generating identity.
- Empty notifications and long Serial Number reads may expose client assumptions → independent-client hardware tests at default MTU.
- Seven-day availability is a hardware acceptance requirement → document a continuous run and daily reconnect procedure; host tests/builds cannot establish it.

## Migration Plan

Add components and tests, integrate startup and publication, intentionally review configuration changes, and build with ESP-IDF 6.1. Flash only on an explicitly selected device when requested. Verify first-boot identity persistence and unpaired access with an independent BLE client before Android integration. Roll back firmware without erasing NVS; older firmware ignores the identity namespace. Android can adopt the documented UUIDs/schema in a separate change.

## References

- [Bluetooth Assigned Numbers](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/index-en.html): standard DIS and Serial Number identifiers.
- [Device Information Service](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/DIS_v1.2/out/en/index-en.html): Serial Number semantics.
- [Espressif random number generation guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/api-reference/system/random.html): entropy prerequisites; implementation must check the installed compatible 6.1 SDK APIs.

- [Nanopb API reference](https://jpa.kapsi.fi/nanopb/docs/reference.html): generator options, bounded buffer streams, and encoding APIs.
