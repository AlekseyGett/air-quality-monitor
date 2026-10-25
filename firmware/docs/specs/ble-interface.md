# BLE interface

## Identity

Device identity is a UUIDv4 stored as a 16-byte NVS blob in namespace
`aqm_identity`, key `uuid`. Only an absent key triggers generation. The UUID is
committed before BLE starts and remains stable through normal resets and firmware
updates that preserve NVS. A deliberate full flash/NVS erase loses the identity.
Initialization, read, validation, write, or commit failures log an identity error
and disable BLE for that boot; sensor acquisition and serial output continue if
sensor configuration is valid. Storage is never automatically erased, corrupt
identity is never silently replaced, and an uncommitted identity is not exposed.
Restart after deliberate storage repair to retry. Measurements are never stored
in NVS. Generation uses ESP32 hardware randomness with the internal entropy source
enabled before RF startup and disabled before BLE initialization.

## Payload and freshness

The authoritative schema is [measurements.proto](../../../protocol/measurements.proto).
Each characteristic value is one complete protobuf `Measurements` message, with
no length prefix or envelope. Fields replace the entire previous snapshot:

| Field | Type | Unit | Availability |
| --- | --- | --- | --- |
| temperature (1) | optional float | degrees Celsius | Absent when invalid |
| humidity (2) | optional float | percent | Absent when invalid |
| pressure (3) | optional int32 | whole hPa | Absent when invalid |

Pressure is rounded directly from internal hPa, nearest integer, positive halves
upward: 1013.25 → 1013; 1013.5 → 1014. Zero remains present when valid. No sea-level
correction is applied. Non-finite or unrepresentable values are omitted. The
example 23.4 °C, 45.6 %, 1013 hPa encodes as
`0d3333bb41156666364218f507` (float values have normal float32 approximation).
An empty message is zero bytes and clears every reading; it is not an error or
an encoded zero measurement. No invalid field retains its previous value.

Every completed acquisition cycle replaces the snapshot and attempts notification,
including unchanged readings and failures, normally every five seconds. Reads do
not trigger acquisition. Initially, after complete sensor failure, or at least
15 seconds after the last completed cycle, reads return an empty message. Valid
readings recover automatically with sensor recovery. Encoding failures publish an
empty snapshot rather than partial bytes or previous readings.

Clients clear absent fields, hide readings on disconnect, and expire each field
15 seconds after its last valid receipt. After reconnecting they wait for a new
read/notification; firmware does not replay a history. A read may return a snapshot
less than 15 seconds old. The schema has no device timestamp or sequence number,
so receipt time does not reveal the original acquisition time. Measurements stay
in RAM and serial output; they are never persisted or uploaded.

## Nanopb generation

The vendored nanopb encoding runtime and generated bindings use 0.4.9.1. Generated
files live in `components/ble_measurements/generated/`. Normal ESP-IDF and host
builds use checked-in runtime/bindings without generation or network access.
Do not edit generated files or copy the schema into firmware.

For explicit regeneration, from `firmware/`, use Python 3.14 and the exact tool
versions in `scripts/protobuf-requirements.txt` (nanopb 0.4.9.1, grpcio-tools 1.78.0,
protobuf 6.33.5, with pinned transitive dependencies). grpcio-tools supplies protoc.
For example, with an environment at `/tmp/aqm-proto-tools`:

```sh
python3.14 -m venv /tmp/aqm-proto-tools
/tmp/aqm-proto-tools/bin/python -m pip install -r scripts/protobuf-requirements.txt
PROTO_PYTHON=/tmp/aqm-proto-tools/bin/python sh scripts/generate-protobuf.sh
PROTO_PYTHON=/tmp/aqm-proto-tools/bin/python sh scripts/generate-protobuf.sh --check
PROTO_PYTHON=/tmp/aqm-proto-tools/bin/python sh tests/run-host-tests.sh
```

`--check` regenerates in a temporary directory and compares both files. The host
suite always compiles the actual nanopb runtime; setting `PROTO_PYTHON` additionally
runs an independent Python protobuf decoder and regeneration consistency check.
Generation suppresses timestamps and uses relative schema paths. Bindings use
explicit presence flags and bounded buffer encoding without dynamic allocation.

## Discovery and GATT

The device advertises complete service UUID
`3598c63f-0a6f-4bc0-b339-980b8205ec01` in connectable legacy advertising. Its complete
name `Air Quality Monitor` is in the scan response; use active scanning to obtain
the name. Filter by service UUID rather than name. Read Serial Number after
connection for durable identity. No pairing, authentication, account, or internet
is required: nearby compatible clients can read measurements and identity.

| Service | Characteristic | Properties / encoding |
| --- | --- | --- |
| Device Information `0x180A` | Serial Number String `0x2A25` | Read; canonical lowercase UUID, 36 UTF-8 bytes without NUL |
| Air Quality Measurements `3598c63f-0a6f-4bc0-b339-980b8205ec01` | Measurements `46aa39c5-5657-4122-b845-02154103438c` | Read, Notify; serialized `Measurements` |

The custom service has exactly one characteristic. NimBLE supplies its CCCD
(`0x2902`); enable notifications by writing `01 00`. Standard GAP/GATT services
are also present. Characteristic writes are not supported. Serial Number is
stable and its long read at MTU 23 is handled by NimBLE's Read Blob processing.

One connected client is supported. Advertising stops during a connection and
resumes on disconnect or failed connection. Recoverable advertising errors retry
at one-second intervals; host reset clears connection/subscription state and host
synchronization restarts advertising. Each new connection must subscribe again.
No immediate notification is promised on subscription: read the snapshot or wait
for the next cycle. A generation barrier prevents old pending samples from being
notified into a new subscription/connection.

The acquisition task copies only a bounded RAM snapshot under a short lock; it
does not enqueue into NimBLE, whose SDK event queue can block. A host-context timer
checks for new generations every 100 ms, adding up to one timer interval of normal
publication delay after conversion. It attempts each latest generation once and
coalesces stalled work without history replay. Send/allocation failures retain the
readable snapshot; later cycles continue automatically. Diagnostics for transport
failures are limited to one per five seconds.

## MTU and message size

Preferred ATT MTU is 80, configured in sdkconfig and set before host startup. The
client initiates exchange and should request at least 80. Negotiated MTU is the
smaller peer capacity; preference is not a guarantee. MTU 80 allows 77 bytes of
notification value after three bytes of ATT overhead. Without exchange, MTU 23
allows 20 bytes, enough for all current valid measurements. The unrestricted
schema's nanopb size bound is 21 bytes (negative int32 pressure would need a longer
varint); valid nonnegative int32 pressure limits application payloads to 16 bytes.

Each notification is exactly one complete message. Before allocation/sending, the
firmware checks against negotiated MTU minus three. Oversized notifications are
skipped with a bounded diagnostic; the complete snapshot remains readable and
acquisition continues. No truncation or application segmentation/reassembly is
used. Standard lower-layer BLE fragmentation is handled by the stack. Future
schema changes must keep the generated maximum within the preferred 77-byte
budget (enforced at compile time) or explicitly revise the transport contract.
A smaller-MTU peer may not receive a future larger message; client implementations
must negotiate accordingly. Long reads can retrieve the complete snapshot.

## Build and verification

Activate a compatible ESP-IDF 6.1 environment and run `idf.py build` from firmware.
No target change is needed. Run `sh tests/run-host-tests.sh`; the existing sensor
suite, identity fakes, actual nanopb encoding, snapshot expiry, BLE transport/event
fakes, and application startup/cadence checks are included. These tests do not
establish radio behavior or unattended hardware reliability.

Before a hardware run, record firmware revision, exact board/module, wiring,
selected serial port, BLE client/phone and OS, and tools. Configure the actual
SDA/SCL pins intentionally. Flash/monitor only the selected port. The following
checks are pending until explicitly executed on hardware:

1. On a clean device, read Serial Number without pairing; power-cycle and flash
   without erasing NVS, and verify the same full UUID. In a deliberate test-only
   storage fault, verify BLE stays disabled with a diagnostic while sensor serial
   output continues. Do not erase a user's identity as routine recovery.
2. Discover by custom service UUID and active-scan name, inspect the GATT table,
   and confirm the custom service has one read/notify characteristic and CCCD.
   Use an independent client, with internet disabled and no bond.
3. Retain MTU 23, read all 36 Serial Number bytes via long read, and decode a
   measurement. Negotiate MTU 80 and confirm a single complete notification per
   normal five-second cycle. Compare pressure in whole hPa with serial output.
4. Subscribe and observe at least one minute, including unchanged measurements.
   Inject partial invalidity with a test harness: absent fields must clear prior
   values while independent valid fields remain. Disconnect/reconnect the sensor:
   verify empty (zero-length) notifications on failure and fresh values on return.
5. Suppress acquisition updates in a test harness while retaining BLE: reads at
   the 15-second boundary must be empty and clients must hide stale values. Verify
   valid updates restore readings automatically.
6. Disconnect/reconnect the BLE client repeatedly and simulate a failed connection;
   verify advertising resumes, subscriptions do not carry over, no old pending
   notification is replayed, and acquisition cadence continues. Test invalid
   sensor wiring configuration separately: BLE still advertises empty snapshots.
7. With a test-only larger payload harness, verify 20/21-byte boundaries at MTU 23
   and 77/78-byte boundaries at MTU 80. Oversized values must never be truncated or
   split; acquisition and readable snapshots must remain available. Restore the
   production schema/build after fault injection.
8. Keep the continuously powered device measuring for seven consecutive days.
   An external observer may record verification evidence. Disconnect and reconnect
   a BLE client daily to confirm fresh readings. Record interruptions, restarts,
   and manual recovery; a break in measurement or BLE availability fails acceptance.

Current execution results belong in the change's verification record. No hardware
or seven-day result follows from passing host tests or a successful build.
