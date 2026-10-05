# Air Quality Monitor — User Flows

## Purpose and Actors

These flows describe the intended first-release experience defined in the
[product vision](vision.md) and [product requirements](requirements.md). They do
not describe functionality already implemented or prescribe screen layouts.
Requirement identifiers below refer to the product requirements.

The primary actor is a DIY home user who prepares one ESP32-based device with a
BME280 sensor, then uses an Android phone for everyday monitoring. A contributor
may build the firmware and application from source or use another compatible BLE
client to read measurements through the public interface.

Everyday measurement, discovery, connection, and viewing work without internet
access, an account, a backend, or a third-party service. Initial tool and software
downloads are separate from everyday operation. Measurements are held only
transiently: neither component stores them in files, databases, or persistent logs,
and the product does not upload them. Remembering the selected device is permitted.
These constraints apply to all flows (DATA-01–03).

## 1. Prepare the Device and Application

**Actor and goal:** A DIY user prepares a working monitoring device and Android
application. A contributor can reproduce both software builds.

**Preconditions:** The user has access to the reference hardware and a BLE-capable
phone running Android 7.0 (API 24) or later.

1. Consult the reference-build instructions for the exact board and sensor module,
   component list, power requirements, wiring, and placement guidance.
2. Follow the software-build instructions for prerequisites, tool versions, and
   commands. A contributor starts from a clean checkout and builds both components.
3. Assemble, flash, and initially configure the device using the reference
   instructions; prepare the Android application for use on the phone.
4. Place the sensor as documented and connect continuous external power.
5. Continue with first connection to confirm that current readings are available.

**Outcome and alternatives:** The powered device measures every five seconds and
is available for BLE access. If setup does not produce a discoverable device or
valid readings, use flows 8 and 4 to distinguish connection and measurement
problems. Exact hardware variants, commands, and setup procedures belong in the
technical instructions required by DOC-01–02; those instructions are not yet
present in `docs`.

**Requirements:** DOC-01–03, MEAS-01, DATA-01. Seven-day unattended operation
(REL-01) is an acceptance criterion, not a setup step or a requirement to keep the
phone connected.

## 2. Connect for the First Time

**Actor and goal:** A user selects a nearby monitoring device for live viewing.

**Preconditions:** The application has no remembered selection. The prepared
device is powered and within BLE range.

1. Open the application and keep it in the foreground.
2. Enable Bluetooth and grant the permissions needed for BLE access when requested.
3. Discover compatible nearby devices. The application clearly identifies the
   candidates so the user can choose one.
4. Select a device. The application remembers its identity and indicates that it
   is connecting; BLE pairing and phone authentication are not required.
5. On success, the application indicates that it is connected. Measurements remain
   unavailable until valid updates arrive.
6. Continue to live viewing as readings become available.

**Outcome and alternatives:** One device is selected and connected. If prerequisites,
discovery, or connection fail, follow flow 8. Nearby compatible BLE clients can
also read the measurements without pairing; selection does not make access private
to this phone.

**Requirements:** CONN-01–02, CONN-04–06, MEAS-03, DOC-03.

## 3. View Current Measurements

**Actor and goal:** A user checks current indoor conditions and observes changes.

**Preconditions:** The application is in the foreground and connected to the
selected device.

1. Wait for valid measurement updates from the device.
2. Read temperature in degrees Celsius (°C) and relative humidity in percent (%),
   each to one decimal place, and atmospheric pressure in hectopascals (hPa) as an
   integer without a decimal point.
3. Keep the application open to see readings refresh at the fixed five-second
   cadence. The interval cannot be changed in the application.
4. Observe subsequent readings, for example after changing conditions in the room.

**Outcome and alternatives:** The user can see current measured conditions. Display
precision does not imply sensor accuracy. Comfort bands, air-quality scores, and
health assessments are outside the MVP. Use flow 4 for unavailable readings and
flow 6 for a lost connection. There is no saved history or export to revisit later.

**Requirements:** MEAS-01–04, DATA-01–03, DOC-03.

## 4. Handle Unavailable Readings

**Actor and goal:** A user understands missing measurements while connected and
can recognize recovery.

**Preconditions:** The application is in the foreground and connected, but one or
more measurements have not arrived, are invalid, or have stopped updating.

1. The application shows the affected readings as unavailable until valid updates
   arrive after connection.
2. If an invalid reading or sensor failure occurs, the affected measurement is
   shown as unavailable, never as a fabricated value such as a substituted zero.
3. If a reading receives no valid update for 15 seconds, it becomes unavailable.
   Other valid, fresh measurements may remain visible.
4. The user can distinguish the connected state with unavailable measurements
   from a disconnected device.
5. When valid updates resume, the affected readings recover automatically without
   device reselection.

**Outcome and alternatives:** Only valid, fresh measurements are presented as
current. If the BLE connection is lost, follow flow 6; sensor recovery is not a
reason to display values from an earlier connection.

**Requirements:** MEAS-03–04, CONN-04, DOC-03.

## 5. Leave and Return to the Application

**Actor and goal:** A user resumes viewing without selecting the same device again.

**Preconditions:** A device has been selected and its identity remembered.

1. Leave the application foreground, for example by switching to another app.
2. The application stops live updates, connection attempts, and the BLE connection.
   Disconnection hides measurements; the powered device continues measuring.
3. Return to the application, or open it again after an application restart.
4. The application uses the remembered selection to attempt reconnection
   automatically while in the foreground.
5. Once connected, it waits for new valid updates before displaying measurements.

**Outcome and alternatives:** Live viewing resumes with fresh readings. Measurements
missed while disconnected are not recovered, and previous readings are not restored
as current. If the device is unavailable or Bluetooth access is blocked, follow
flows 6 and 8. No background monitoring is provided.

**Requirements:** CONN-02–04, MEAS-03, DATA-02, DOC-03.

## 6. Recover from Connection Loss

**Actor and goal:** A user resumes live viewing after a temporary interruption.

**Preconditions:** A device is selected and the application remains in the
foreground.

1. The connection is lost, for example when the device loses power or moves out
   of BLE range.
2. On detecting disconnection, the application immediately hides measurements and
   shows the disconnected state.
3. The application automatically attempts to reconnect to the selected device and
   indicates connecting or connection failure as appropriate.
4. The user restores device power or returns within range when needed.
5. On reconnection, the application indicates that it is connected and waits for
   newly received valid readings before resuming live display.

**Outcome and alternatives:** Viewing resumes without device reselection. Persistent
failures provide a recovery action through flow 8. Leaving the foreground stops
connection attempts as described in flow 5. Retry timing is a technical decision
and is not specified by these flows.

**Requirements:** CONN-02–05, MEAS-03, DOC-03.

## 7. Change the Selected Device

**Actor and goal:** A user replaces the remembered selection with another device.

**Preconditions:** A device is already selected; the application is in the foreground.

1. Choose to change the selected device and discover compatible nearby devices.
2. Select another clearly identified candidate.
3. The application replaces the remembered selection and connects to the new
   device, supporting only one selected device at a time.
4. Measurements from the previous device are hidden. The application displays the
   new device's readings only after receiving valid updates from it.

**Outcome and alternatives:** Subsequent automatic reconnection targets the newly
selected device. An empty discovery result or connection failure follows flow 8.
The MVP does not provide simultaneous monitoring of multiple devices.

**Requirements:** CONN-01–02, CONN-04–05, MEAS-03.

## 8. Resolve Bluetooth, Discovery, or Connection Problems

**Actor and goal:** A user understands why live viewing is unavailable and can
recover where the phone and device support it.

**Preconditions:** The user is discovering devices or trying to connect in the
foreground.

1. The application explains the detected problem and shows the relevant state.
2. The user takes the offered recovery action appropriate to that problem:

   | Problem | User action and expected result |
   | --- | --- |
   | Phone does not support BLE | Use a supported BLE-capable phone; this phone cannot connect. |
   | Bluetooth is disabled | Enable Bluetooth using the offered action, then resume discovery or connection. |
   | Required permissions are missing or denied | Grant permissions using the offered permission or settings action, then resume discovery or connection. |
   | No compatible devices are discovered | Check device power and range, then rescan. |
   | Connection fails | Check device availability and retry using the offered action; automatic foreground reconnection still applies to the remembered selection. |

3. Once the problem is resolved, discover and select a device if needed, or
   reconnect to the remembered selection.
4. Wait for valid fresh updates before viewing measurements.

**Outcome and alternatives:** Recovery leads back to live viewing. Declining a
permission or failing a connection does not crash the application or permanently
prevent a later attempt. Connecting, connected, disconnected, and connection
failure remain distinguishable. Recovery does not require internet access.

**Requirements:** CONN-01–05, MEAS-03, DATA-01, DOC-03.

## 9. Read Measurements with Another Compatible BLE Client

**Actor and goal:** A contributor or user reads the device through its public BLE
interface without depending on the Android application.

**Preconditions:** The device is powered and in range, and the client supports its
documented BLE interface. Concurrent client connections are not required; an
existing client may need to disconnect first.

1. Consult the public BLE interface documentation for compatible-device discovery,
   service and characteristic identifiers, encoding, units, validity, and cadence.
2. Discover the device and connect without pairing or phone authentication.
3. Receive and interpret temperature, relative humidity, and atmospheric pressure
   using the documented interface, including validity information and the
   five-second update cadence.
4. Disconnect when finished.

**Outcome and alternatives:** The client can interpret measurements without
undocumented application-specific knowledge. Connection failures can be retried
after checking power, range, and device availability. Exact identifiers and payloads
belong in the technical documentation required by DOC-04, which is not yet present
in `docs`. Nearby access is open to compatible clients, not restricted to a
remembered or authenticated phone.

**Requirements:** CONN-06, DOC-04, MEAS-01, DATA-01, DATA-03.

## Release Boundaries

These flows cover local, foreground Android monitoring with continuous device
power. History, background monitoring, notifications, in-app device configuration,
additional sensors, remote access, other application platforms, and simultaneous
monitoring of multiple devices are outside the first release. Future directions
are described in the product vision and are not steps in these flows.
