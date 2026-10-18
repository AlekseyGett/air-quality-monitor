# Firmware agent guidance

## Scope and current state

This file applies to the `firmware/` subtree of the Air Quality Monitor repository.
The sibling `android/` directory contains the mobile application.

- This is an ESP-IDF C++ project targeting `esp32`. The tracked `sdkconfig` was
  generated with ESP-IDF 6.1.0; use a compatible 6.1 environment.
- `main/air-quality-monitor.cpp` initializes a shared I2C bus and acquires BME280
  measurements every five seconds with serial output and automatic recovery.
  BLE communication is planned, not implemented.
- The root `CMakeLists.txt` loads ESP-IDF and declares the project;
  `main/CMakeLists.txt` registers the application sources.
- `docs/datasheets/` contains hardware references. `docs/specs/` is the location for
  firmware technical specifications. `build/` is generated and ignored.

## Product requirements

Read the shared [vision](../docs/product/vision.md) and
[requirements](../docs/product/requirements.md) before implementing behavior.
The [user flows](../docs/product/user-flows.md) provide application context.
These documents describe the intended release, not existing functionality.

- The MVP uses one continuously powered ESP32 device with a BME280 to measure
  temperature, relative humidity, and atmospheric pressure every five seconds.
- Current measurements are exposed over BLE without requiring pairing or an
  authenticated client. Core operation requires no internet, account, or backend.
- Do not persist measurements in files, databases, NVS, or persistent logs, or
  upload them. Transient live values in memory are permitted.
- Represent invalid or failed measurements as unavailable; do not substitute
  fabricated values such as zero. Support recovery when valid data resumes.
- The application must hide disconnected readings and treat readings as stale
  after 15 seconds without a valid update. Keep firmware validity and delivery
  semantics compatible with those requirements.
- Seven days of unattended measurement and BLE availability is a product
  acceptance requirement, not something a successful build proves.
- SCD40, SDS011, and SGP40 datasheets are future references; their presence does
  not put additional sensors in the MVP. History, cloud services, Wi-Fi features,
  and in-app device configuration are also outside the MVP.

## Build and hardware workflow

Run commands from `firmware/`. Activate the installed ESP-IDF environment first,
for example with `. /path/to/esp-idf/export.sh`, replacing the path with the local
installation. Do not hard-code a contributor's absolute SDK path in project files.

```sh
idf.py build
idf.py menuconfig
idf.py -p <PORT> flash monitor
```

- Use `build` to validate code and build-system changes.
- Use `menuconfig` only for intentional configuration changes and review the
  resulting `sdkconfig` diff. The configuration is tracked despite its generated
  header; do not hand-edit it casually or regenerate it unnecessarily.
- Replace `<PORT>` with the actual serial port. Flash and monitor hardware when
  requested by the task; do not assume a connected board or select a port blindly.
- Keep the existing ESP32 target. Do not run `set-target` as routine setup, since
  it can reset configuration.
- If the SDK or hardware is unavailable, report the limitation and the checks
  that remain unverified.

## Implementation conventions

- Follow the existing C++ style: four-space indentation and braces on separate
  lines. Preserve C linkage for `app_main`.
- Preserve the ESP-IDF initialization order in the root CMake file. Register new
  sources and required component dependencies in the appropriate component's
  `CMakeLists.txt`.
- Keep implementations small and focused on the requested behavior. Introduce
  reusable components when needed rather than imposing an architecture on the
  current skeleton.
- C++ exceptions and RTTI are disabled in the tracked configuration. Do not rely
  on them without a deliberate configuration and design change.
- Use ESP-IDF error handling and logging facilities where appropriate. Handle
  recoverable sensor and connection failures without preventing later recovery.
  Avoid busy loops; use appropriate FreeRTOS timing and synchronization.
- Consult the relevant datasheet before implementing sensor commands, timing,
  compensation, or validity checks. Exact board/module variants and wiring are
  not yet specified; do not invent GPIO assignments or electrical assumptions.
- BLE identifiers and payloads are not yet defined. When introducing the public
  interface, document discovery, service and characteristic identifiers, encoding,
  units, validity, and update cadence in `docs/specs/`. Coordinate changes affecting
  Android compatibility rather than introducing undocumented interpretations.

## Verification and change hygiene

- Run focused firmware host tests with `sh tests/run-host-tests.sh`. They use the
  real Bosch compensation code with fake ESP-IDF transport and timing boundaries.
  No firmware formatter configuration or firmware CI currently exists. The
  repository's Android CI and pre-commit checks do not validate this firmware.
  Do not present Android checks as firmware coverage.
- For code or build changes, run `idf.py build` when the toolchain is available.
  Add focused tests for meaningful new behavior where practical, and document
  relevant hardware checks for sensor and BLE work.
- Separate compilation, automated tests, and observed hardware behavior in the
  result. Do not claim hardware or seven-day acceptance checks were performed
  unless they actually ran.
- For documentation-only changes, verify paths and commands and run
  `git diff --check`; a firmware build is unnecessary.
- Keep generated build outputs and machine-specific paths out of commits. Review
  configuration diffs, preserve unrelated work, and avoid changing Android or
  shared product requirements unless the task calls for it.
