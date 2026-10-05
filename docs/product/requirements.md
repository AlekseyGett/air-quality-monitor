# Air Quality Monitor — Product Requirements

## Purpose and Release Scope

This document defines the intended first release, based on the
[product vision](vision.md). It specifies required behavior and acceptance criteria;
it does not describe functionality already implemented.

The MVP serves DIY home users who assemble and flash one ESP32-based monitoring
device with a BME280 sensor, then view current readings in an Android application
over Bluetooth Low Energy (BLE). The reference device uses continuous external
power. The application supports Android 7.0 (API 24) and later on phones with BLE.

Measurement persistence and history are deferred to a future release with the
optional backend. Historical charts, export, background monitoring, simultaneous
monitoring of multiple devices, in-app device configuration, additional sensors,
notifications, remote access, and other application platforms are outside the MVP.

## Functional Requirements

### Measurement and Live Display

- **MEAS-01 — Measurement cadence:** The device must measure temperature, relative
  humidity, and atmospheric pressure at a fixed five-second interval. The interval
  is not configurable in the application.
- **MEAS-02 — Presentation:** While connected in the foreground, the application
  must display and refresh valid temperature readings in degrees Celsius (°C) and
  relative humidity readings in percent (%), both to one decimal place. Atmospheric
  pressure must be displayed in hectopascals (hPa) as an integer, without a decimal
  point. Display precision does not imply sensor accuracy. Comfort bands,
  air-quality scores, and health assessments are outside the MVP.
- **MEAS-03 — Freshness:** On connection, readings must remain unavailable until
  valid updates arrive. A reading must become unavailable after 15 seconds without
  a valid update. Disconnecting must immediately hide measurements. Previously
  received values must never be presented as current after reconnecting.
- **MEAS-04 — Sensor failures:** Invalid readings and sensor failures must be shown
  as unavailable, never as fabricated values such as a substituted zero. Valid
  measurements may remain visible when another measurement fails. Display must
  recover automatically when valid updates resume.

### Discovery and Connection

- **CONN-01 — Device selection:** The application must discover compatible nearby
  devices and allow the user to select one. It must clearly identify candidates,
  support one selected device at a time, and allow changing that selection.
- **CONN-02 — Remembered device:** The application must retain the selected device's
  identity across application restarts. While in the foreground, it must attempt
  to reconnect automatically on return to the application and after temporary
  connection loss, without requiring device selection again.
- **CONN-03 — Foreground lifecycle:** Leaving the foreground must stop live updates,
  connection attempts, and the BLE connection. Returning to the foreground must
  resume connection to the selected device. There is no background monitoring
  guarantee or requirement to recover measurements missed while disconnected.
- **CONN-04 — Connection feedback:** The interface must distinguish connecting,
  connected, disconnected, and connection failure states. A connected device with
  unavailable measurements must be distinguishable from a disconnected device.
- **CONN-05 — Recoverable problems:** The application must explain unsupported BLE,
  disabled Bluetooth, missing permissions, no discovered devices, and connection
  failures. Where recovery is possible, it must offer an appropriate action, such
  as granting permission, enabling Bluetooth, rescanning, or retrying. Denial or
  failure must not crash the application or permanently prevent later connection.
- **CONN-06 — Nearby access:** Reading measurements must not require BLE pairing or
  an authenticated phone. Documentation must explain that nearby compatible BLE
  clients can read the device's measurements. Concurrent client connections are
  not required.

## Offline Operation, Privacy, and Reliability

- **DATA-01 — Independent operation:** Measurement, discovery, connection, and live
  viewing must work without internet access, an account, a backend, or a
  third-party service. Initial downloads of tools and software are separate from
  everyday operation.
- **DATA-02 — No measurement persistence:** Neither the device nor the application
  may persist measurements or provide historical retrieval in the MVP. Live values
  may be held transiently in memory. Remembered device identity is permitted;
  measurement values must not be written to files, databases, or persistent logs.
- **DATA-03 — No external transmission:** The product must not upload measurements
  or require telemetry or analytics services for core functionality. This does not
  restrict the explicitly supported nearby BLE access in CONN-06.
- **REL-01 — Unattended operation:** With continuous external power, the reference
  device must continue measuring and remain available for BLE access for at least
  seven consecutive days without manual restart or recovery. Battery endurance is
  outside the MVP.

## Documentation and Interfaces

- **DOC-01 — Reference build:** Provide a reproducible reference build with exact
  ESP32 development-board and BME280 module variants, a component list, power
  requirements, wiring, flashing, initial setup, and sensor placement guidance.
  General compatibility with other boards or modules is not required.
- **DOC-02 — Software builds:** Document prerequisites, tool versions, and commands
  that let an external contributor build the firmware and Android application
  from a clean checkout.
- **DOC-03 — Everyday use:** Explain permissions, selecting a device, automatic
  reconnection, foreground-only operation, unavailable readings, and connection
  recovery. State the absence of measurement storage and history.
- **DOC-04 — Public BLE interface:** Document how a client identifies a compatible
  device and receives the three measurements, including service and characteristic
  identifiers, payload encoding, units, validity, and update cadence. Exact payload
  and transport design belongs in technical documentation; it must support these
  requirements and use no undocumented app-specific interpretation.

## Acceptance Verification

Record the firmware and application revisions, reference hardware, Android phone
and OS version, procedures, and results for each acceptance run. The following
checks validate product behavior; detailed technical test scripts may live with
the implementation.

| Scenario | Acceptance criteria |
| --- | --- |
| Reference setup and builds | Follow the documented steps from a clean checkout to build both components, assemble and flash the reference device, and obtain readings without undocumented setup steps (DOC-01–03). |
| Live measurements | Discover and select the reference device; observe all three values with the required units and precision. Verify five-second measurement and delivery cadence over at least one minute (MEAS-01–02, CONN-01). |
| Freshness and sensor failure | Suppress valid updates while retaining the connection; readings become unavailable at 15 seconds. Inject invalid data or a sensor failure; no fabricated value is shown. Restore valid updates and verify automatic recovery (MEAS-03–04). |
| Connection loss and return | Move out of range or power off the device; readings disappear when disconnection is detected. Restore availability while the app stays open; it reconnects automatically and displays only newly received readings (CONN-02, MEAS-03). |
| Application lifecycle | Leave the foreground and verify updates, connection attempts, and BLE connection stop. Return and verify reconnection. Restart the app and verify remembered selection without restored measurement values (CONN-02–03, DATA-02). |
| Bluetooth and permissions | Test disabled Bluetooth, denied permissions, an empty scan, a connection failure, and a phone without BLE support. Verify understandable states, no crashes, and successful recovery where supported (CONN-04–05). |
| Nearby BLE access | Read measurements using a compatible client without pairing. Verify the documented interface is sufficient to interpret values (CONN-06, DOC-04). |
| Offline operation and storage | Disable internet access while keeping BLE enabled; verify discovery and live readings. Inspect product storage and logging paths to confirm no measurement persistence or external uploads (DATA-01–03). |
| Seven-day reliability | Keep the reference device continuously powered for seven consecutive days. Use a test observer to verify continuing five-second measurements, and disconnect and reconnect a BLE client each day to verify fresh readings. Record any interruption, restart, or recovery; manual recovery or a break in measurement or BLE availability fails acceptance (REL-01). |

An external test observer may record measurements as verification evidence; this
does not introduce measurement persistence into the product.

## Chosen Defaults and Limits

The five-second interval, 15-second stale threshold, one-decimal display precision
for temperature and relative humidity, integer hPa display for atmospheric pressure,
and API 24 minimum are the initial release defaults. BLE identifiers, payloads,
retry timing, exact reference board/module variants, and technical test tooling
must be documented during implementation. They are not additional user-facing
features. Sensor accuracy is constrained by the documented reference hardware;
this release makes no calibrated accuracy or medical suitability claim.
