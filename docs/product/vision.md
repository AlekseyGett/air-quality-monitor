# Air Quality Monitor — Product Vision

## Purpose

Make indoor environmental monitoring accessible, understandable, and convenient,
while keeping users in control of their data.

Air Quality Monitor is an open-source project that aims to help people understand
current conditions in their homes and make informed decisions
about improving their indoor environment.

The project is in early development. This document describes the intended product
and the goals for its first release; it does not describe a finished system.

## User Needs

People need a practical way to see what is happening in their indoor environment
and how conditions respond to changes in their surroundings. Current measurements
should be easy to access and understand.

The project addresses the need for affordable hardware, clear setup instructions,
a pleasant everyday experience, and monitoring that works without an internet
connection or a dependency on third-party services.

## Target Audience

The initial audience is DIY home users: homeowners and renters who are willing to
assemble and flash a monitoring device using documented instructions, then use a
mobile application for everyday monitoring.

The project also welcomes technology enthusiasts and contributors interested in
open-source software, hardware experimentation, self-hosting, and future smart-home
integrations. Documented interfaces and reproducible builds should make it possible
for others to understand, modify, and extend the system.

## Product Overview

The intended system has two core components:

- **Monitoring device:** An ESP32-based device that measures indoor environmental
  conditions and exposes readings over Bluetooth Low Energy (BLE).
- **Mobile application:** An application that connects to the device over BLE and
  presents current measurements while connected to the device.

An optional backend is a future extension for measurement persistence, historical
views, and remote access when the device is outside BLE range. The first release
does not store measurements or provide history. Core local monitoring must remain
usable without a backend.

The first release will measure temperature, relative humidity, and atmospheric
pressure. These readings provide an initial view of indoor conditions. Pollutant
monitoring and broader air-quality insights will require additional sensors and
are future directions.

## Product Goals

- **Accessible monitoring:** Use affordable hardware and provide instructions that
  help DIY users assemble and configure a working system.
- **Useful information:** Present current readings clearly so users can understand
  their environment and observe the effects of changes to their surroundings.
- **Convenient everyday use:** Make connecting to the device and reviewing
  measurements straightforward after initial setup.
- **Privacy and data ownership:** Let users retain control over their measurements
  and use core functionality without cloud services.
- **Reliable operation:** Support extended monitoring with minimal user
  intervention.

## Product Principles

- **Privacy first:** Core functionality must not require users to share their
  environmental data with third parties.
- **Offline first:** Essential monitoring functionality must remain available
  without internet connectivity.
- **Usability matters:** Provide clear setup guidance and an understandable,
  consistent interface for everyday use.
- **Simplicity:** Prefer understandable solutions and add complexity only when it
  serves a clear user need.
- **Open by design:** Make the system understandable and extensible through open
  source, documented communication protocols, and open standards where practical.
- **Incremental development:** Deliver useful functionality in small,
  independently verifiable increments.

## Initial Release (MVP)

The first release will provide a complete local monitoring experience built around:

- One ESP32-based monitoring device with a BME280 sensor and continuous external
  power.
- Temperature, relative humidity, and atmospheric pressure measurements.
- BLE connectivity between the device and the Android application.
- Current readings refreshed every five seconds while the Android application is
  in the foreground and connected to the device.
- Remembered device selection and automatic reconnection during foreground use.
- Documented device assembly, initial configuration, and software builds.

Measurement persistence, history, background monitoring, additional sensors,
notifications, in-app device configuration, remote access, and other platforms are
outside the initial release. BLE readings are accessible without pairing; nearby
compatible clients can read them.

## Initial Success Criteria

The first release will be considered successful when:

- A DIY user can assemble, flash, and configure a device by following the project
  documentation.
- A user can connect the Android application to the device over BLE and view
  current temperature, relative humidity, and atmospheric pressure readings.
- Core measurement and live viewing functionality work without an
  internet connection or backend.
- With continuous power, the device measures and remains available for BLE access
  for at least seven consecutive days without a manual restart or recovery.
- An external contributor can build the firmware and Android application using
  the provided documentation.

The seven-day criterion measures unattended reliability. Battery endurance will be
specified separately if battery operation is introduced. Detailed acceptance
procedures belong in the product requirements. These criteria will be refined as
the project evolves.

## Future Directions

Potential extensions include:

- Multiple monitoring devices.
- CO₂ concentration, particulate matter (PM2.5 and PM10), and volatile organic 
  compound (VOC) monitoring.
- Notifications that help users notice unfavorable measured conditions.
- In-app device configuration, such as changing the measurement interval.
- Wi-Fi connectivity and an optional backend for measurement persistence,
  historical views, and remote monitoring.
- Wear OS, iOS, and web applications.
- Data export and smart-home integrations.

These are possible directions, not committed features. Future work should follow 
user needs and the product principles above.
