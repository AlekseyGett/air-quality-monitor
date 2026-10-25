# BLE Measurements Spec Delta

## Purpose

Let nearby compatible clients discover the monitoring device and obtain current environmental measurements through a documented protobuf BLE interface.

## ADDED Requirements

### Requirement: Measurement service interface
The firmware SHALL expose Air Quality Measurements Service `3598c63f-0a6f-4bc0-b339-980b8205ec01` with exactly one characteristic, `46aa39c5-5657-4122-b845-02154103438c`, supporting read and notify without write access. Access SHALL require neither pairing nor authentication. Standard BLE infrastructure services and the notification subscription descriptor SHALL remain permitted.

#### Scenario: Discover measurement interface
- **WHEN** an unpaired client discovers the custom service
- **THEN** it finds exactly one read/notify characteristic with the specified UUID and a notification subscription descriptor
- **AND** it can read and subscribe without pairing or authentication

### Requirement: Discoverability and reconnection
After successful identity and BLE initialization, the firmware SHALL advertise the custom service UUID and a human-readable device name. It SHALL support at least one client and resume connectable advertising after disconnection or a failed connection attempt. BLE connection state SHALL NOT stop sensor acquisition.

#### Scenario: Client reconnects
- **WHEN** a connected client disconnects
- **THEN** advertising resumes and a client can reconnect without restarting the device
- **AND** acquisition continues on its existing schedule

### Requirement: Protobuf encoding and units
Every characteristic value SHALL be one serialized `Measurements` message from `../protocol/measurements.proto`, without an additional envelope. Present temperature and humidity SHALL use float degrees Celsius and percent. Present pressure SHALL use int32 hPa, rounded to the nearest integer with positive half values rounded upward. Non-finite or unrepresentable values SHALL be omitted.

#### Scenario: Encode physical values
- **WHEN** a cycle yields 23.4 degrees Celsius, 45.6 percent humidity, and 1013.25 hPa internally
- **THEN** the message contains temperature 23.4, humidity 45.6, and pressure 1013

#### Scenario: Preserve presence of zero
- **WHEN** a valid temperature equals zero degrees Celsius
- **THEN** the temperature field is present with value zero

### Requirement: Notification MTU and message integrity
The firmware SHALL configure preferred ATT MTU 80. Each measurement notification SHALL contain one complete protobuf message no larger than the negotiated ATT MTU minus three bytes. It SHALL NOT truncate or fragment messages at application level. If a message exceeds this capacity, it SHALL skip that notification, report a diagnostic, retain the complete readable snapshot, and continue acquisition.

#### Scenario: Preferred MTU negotiated
- **WHEN** the client negotiates ATT MTU 80
- **THEN** messages up to 77 bytes are sent as single complete notifications

#### Scenario: Client retains default MTU
- **WHEN** the connection retains ATT MTU 23
- **THEN** current messages of up to 20 bytes can be sent without requiring MTU exchange

#### Scenario: Message exceeds negotiated capacity
- **WHEN** a serialized message exceeds the negotiated ATT MTU minus three bytes
- **THEN** no truncated or segmented notification is sent
- **AND** the complete readable snapshot remains available and acquisition continues

### Requirement: Snapshot availability
Each value SHALL replace the entire previous snapshot. Unavailable fields SHALL be absent, independently valid fields SHALL remain present, and a cycle with no valid fields SHALL produce the empty protobuf message. Before the first completed cycle, and when the latest cycle is at least 15 seconds old, reads SHALL return an empty message rather than old measurements.

#### Scenario: Partial sensor failure
- **WHEN** a new cycle has unavailable humidity and valid temperature and pressure
- **THEN** its snapshot omits humidity and contains the two valid current fields
- **AND** clients interpreting the snapshot clear previous humidity

#### Scenario: No current measurements
- **WHEN** no cycle has completed, the latest cycle failed completely, or it is at least 15 seconds old
- **THEN** a read returns a zero-length protobuf message with all fields absent

### Requirement: Cycle delivery
After each completed acquisition cycle, the firmware SHALL atomically replace its transient readable snapshot and attempt notification to subscribed clients, including for partial or complete failures and numerically unchanged samples. Reads SHALL NOT trigger sensor acquisition. Delivery failures SHALL NOT block acquisition or replay a history backlog.

#### Scenario: Healthy notifications
- **WHEN** a client is subscribed and acquisition continues normally
- **THEN** it receives one complete current message per five-second cycle, including unchanged readings

#### Scenario: Failure and recovery
- **WHEN** a valid cycle is followed by a failed cycle and then a recovered cycle
- **THEN** subscribed clients are sent respectively the valid snapshot, an empty message, and the fresh recovered snapshot

#### Scenario: New subscription
- **WHEN** a client reconnects and subscribes
- **THEN** no queued measurements from the previous connection are replayed
- **AND** the client can read a fresh current snapshot or await the next cycle notification

### Requirement: Public contract and transient storage
The firmware SHALL document discovery, both service interfaces, UUIDs, protobuf field presence, units, rounding, snapshot freshness, subscription and reconnect behavior in `docs/specs/ble-interface.md`. Measurement values SHALL remain in transient memory and SHALL NOT be persisted or uploaded.

#### Scenario: Independent client implementation
- **WHEN** a contributor uses the public interface documentation and shared protobuf schema
- **THEN** they can discover the device, read its identity, and interpret measurements without undocumented Android-specific assumptions
