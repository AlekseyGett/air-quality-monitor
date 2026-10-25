# Verification

## Automated checks — 2026-10-21

- ESP-IDF 6.1, ESP32: `idf.py build` passed. Final binary size 0x73c30 bytes;
  55% of the configured 1 MiB application partition remains free.
- `sh tests/run-host-tests.sh` with `PROTO_PYTHON` pointing to the pinned Python
  3.14 generation environment passed. This includes the unchanged sensor/Bosch
  suite, identity storage and entropy boundaries, actual nanopb encoding,
  independent Python protobuf decoding, generation consistency, snapshot expiry,
  production BLE code compiled against SDK fakes, and production application
  startup/cadence fault injection.
- BLE host tests cover service properties, UUID registration, long-read callback
  behavior, empty snapshots/notifications, unchanged values, subscription changes,
  no queued replay across reconnect, MTU boundaries, mbuf allocation and send
  failures, advertising failures/retry, failed connection, and host resync.
- `openspec validate add-ble-services --strict` passed.
- `git diff --check` passed. Configuration review found only the preferred ATT
  MTU change from 256 to 80 and its compatibility alias.

## Implementation detail found during SDK review

ESP-IDF's FreeRTOS NimBLE `eventq_put` uses `portMAX_DELAY`. To preserve bounded
acquisition, publication writes only the protected latest snapshot/generation.
A 100 ms NimBLE host callout detects new generations and sends the latest once.
There is no potentially blocking queue insertion in the acquisition task, no
measurement backlog, and no application payload fragmentation.

The standard DIS is registered locally with only Serial Number, rather than
initializing the built-in DIS alongside another instance. NimBLE handles Read
Blob offsets by slicing the full value returned by the read callback.

## Hardware checks — not executed

No board/port was selected, no firmware was flashed, and no physical BLE client,
sensor failure injection, power-cycle identity test, or seven-day unattended run
was performed. The procedure is in `docs/specs/ble-interface.md`. Host fakes and
compilation do not prove radio behavior, timing on hardware, or product acceptance.
