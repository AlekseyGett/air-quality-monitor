# BME280 acquisition over shared I2C

This development increment reads a BME280 and prints live values through the
existing 115200 baud UART console. It adds no BLE interface, measurement history,
storage, network upload, or other sensors. Readings exist only in current-cycle
memory and console output. Serial capture by an external test observer is optional.

## Hardware prerequisites and configuration

No reference board, module or wiring has been selected. Before powering or flashing,
confirm the exact ESP32 board and BME280 module, supply and I/O voltage, common
ground, chosen pins, address strap and pull-ups using their documentation. Check
board-reserved pins (including flash, boot strapping and console pins). A GPIO
accepted by ESP-IDF is not necessarily available on a particular board.

The I2C bus requires suitable external SDA/SCL pull-ups to a verified compatible
logic voltage; a module may already include them. Internal pull-ups are disabled.
Do not assume a module's supply voltage equals its I/O voltage or that it has level
translation. Do not change connections live unless the specific hardware and test
procedure permit it.

Use ESP-IDF 6.1 targeting the existing `esp32` configuration. From `firmware/`:

```sh
. /path/to/esp-idf/export.sh
idf.py menuconfig
idf.py build
idf.py -p <PORT> flash monitor
```

Replace the SDK path locally and `<PORT>` with the confirmed serial port. In
**Air Quality Monitor**, set `CONFIG_AQM_I2C_SDA_GPIO` and
`CONFIG_AQM_I2C_SCL_GPIO` to distinct output-capable pins suitable for your board.
Both default to `-1` (unset). Startup rejects unset, equal and unsupported pins
before creating a bus and returns with an actionable diagnostic. Bus creation
failure also ends startup without a retry/reboot loop. No alternate pins are chosen.

Select `CONFIG_AQM_BME280_ADDRESS_76` (default, 0x76) or
`CONFIG_AQM_BME280_ADDRESS_77` (0x77). Match the hardware address strap; the firmware
does not scan or fall back to another address. Review `git diff -- sdkconfig` after
configuration; do not commit contributor-specific pin selections as defaults.

## Ownership and driver contract

`aqm::create_i2c_bus` creates one application-owned master bus with automatic
controller selection and default clock source. `app_main` retains it for the
application lifetime. Each sensor attaches a device to the borrowed bus;
`aqm::Bme280` attaches a seven-bit device at **100 kHz**. It owns only that device
handle and its Bosch state. The instance cannot be copied or moved because callback
context points to it. Destroying it removes only its device. Initialization retries
reuse the device handle and never create, reset or delete the shared bus.

`initialize()` verifies chip identity 0x60, resets the sensor, waits for NVM copying,
loads calibration, checks temperature calibration and configures sleep mode, x1
oversampling on all channels and disabled IIR filter. It verifies configuration
readback. `acquire(Measurement&)` triggers a forced conversion, waits for completion
and returns ESP-IDF errors. `cycle(Measurement&)` initializes when needed, then
acquires. Failure causes initialization to be retried on the next scheduled cycle.
An invalid independent channel can leave the other current fields valid even when
the operation returns `ESP_ERR_INVALID_RESPONSE`. Consumers must use each validity
flag, not the overall error, to decide whether a field is available.

Every sample starts with all validity flags false. No previous sample is cached.
The numeric members of invalid fields have no measurement meaning. Raw temperature
and pressure sentinel `0x80000` and humidity sentinel `0x8000` are rejected before
compensation. Temperature requires T1 other than 0/0xffff and nonzero T2; pressure
requires P1 other than 0/0xffff; humidity requires H2 other than 0/-1. These are
conservative integrity checks for erased/degenerate calibration, not a checksum
guarantee against arbitrary NVM corruption. Unusable temperature calibration or
temperature raw data makes all dependent channels unavailable. Bad pressure or
humidity calibration affects only its channel. Non-finite compensated outputs
are rejected independently; non-finite temperature invalidates both dependents.
Valid zero degrees Celsius remains valid.

The unmodified Bosch double precision implementation supplies factory compensation.
Output units are Celsius, relative humidity percent, and compensated pressure in
Pa divided by **100.0** to obtain local hPa. No sea-level/altitude correction is
applied. Bosch's normal physical-range clamping remains in effect for otherwise
valid inputs; raw sentinel checks prevent disabled inputs from being accepted
merely because compensation clamps them.

## Timing, errors and presentation

Combined register-write/read transactions and register-prefixed writes have a
maximum timeout of 100 ms. Initialization and conversion each have a 100 ms elapsed
budget; callbacks reduce transaction timeouts to the remaining budget. Bosch reset
polling is finite and also guarded by this budget. FreeRTOS blocking waits round
up to a tick, so return time can exceed a deadline by scheduler resolution/latency.
There are no busy conversion loops. `bme280_cal_meas_delay` derives a 9300 µs x1
conversion wait, rounded upward to a tick, followed by bounded polling of status
`measuring`/`im_update`. Timed-out conversions are never read as valid samples.
No application-wide bus lock is held during sensor conversion.

The first cycle runs immediately, initializing and sampling without an added
five-second sleep. Subsequent attempts use absolute uptime deadlines spaced five
seconds apart, anchored at the initial attempt. Conversion/printing time does not
accumulate into the schedule. After a late cycle, elapsed starts are skipped in
favor of the next future deadline, without catch-up bursts. Clock calculations use
64-bit microsecond uptime; FreeRTOS delays remain short and bounded.

One `ESP_LOGI` result line per cycle uses the existing RTOS uptime timestamp:

```text
I (1234) aqm: Temperature: 23.4 °C, Humidity: 45.6 %, Pressure: 1013 hPa
I (6234) aqm: Temperature: 23.4 °C, Humidity: unavailable, Pressure: 1013 hPa
```

Temperature and humidity use one decimal place; pressure is rounded to whole hPa
only when formatted. Failed cycles print explicit `unavailable` fields and at most
one application warning explaining the ESP-IDF-style error. ESP-IDF may also emit
transport diagnostics. Missing sensors, incompatible identity, failed transactions
and stuck conversions are recoverable on later five-second attempts without a
restart. A device holding bus lines low may need hardware intervention; global
shared-bus reset/recovery is outside this increment.

## Automated verification

Run `sh tests/run-host-tests.sh` with a C99 compiler and C++17 compiler. The script
builds in a temporary directory and removes its outputs. It compiles the real Bosch
code and project adapters with fake ESP-IDF I2C lifecycle/transport and a 10 ms
FreeRTOS clock. Tests cover configuration rejection, borrowed bus identity and
ownership, repeated initialization and cleanup, both addresses, known calibration
and raw fixtures, physical units, independent channel failures, zero temperature,
non-finite values, missing/wrong devices, transfer failure/timeouts, stuck reset
and conversion, recovery, fresh-cycle clearing, formatting and skipped deadlines.
Bus creation/reset/deletion counters and handle counts fail on sensor ownership
violations. The fake GPIO mask matches ESP32's SDK output-capable mask; this does
not validate a board's wiring.

The fixed raw/calibration fixture is evaluated against the datasheet appendix 8.1
formulas independently of the vendored code. Its temperature is 25.0824779 °C and
pressure 1006.532581448 hPa (integer `t_fine` 128422). Humidity uses a synthetic
calibration fixture and an independently evaluated appendix 8.1 expression.

Run `idf.py build` separately with ESP-IDF 6.1. Automated tests and successful
compilation do not prove electrical behavior or seven-day product reliability.

## Manual hardware verification (not yet performed)

Record firmware revision, SDK version, actual board/module, supply and logic
voltages, external pull-up values/source, SDA/SCL pins, address strap, serial port
and test observer. Confirm all electrical prerequisites above before proceeding.

1. Configure confirmed pins/address, build and flash using the explicit port.
   Observe the first result after initialization and at least one minute of
   approximately five-second result/start cadence (allow scheduler resolution).
   Check Celsius, percent and local hPa against a suitable reference; display
   precision is not a claim of sensor accuracy.
2. Repeat with hardware/fixture supporting each of 0x76 and 0x77. Confirm matching
   configuration succeeds and mismatched configuration stays unavailable without
   silently scanning to the other address.
3. Boot with the sensor absent using a safe powered-down wiring procedure or a
   suitable fault fixture. Confirm unavailable results and bounded retries every
   five seconds. Restore communication by a procedure confirmed safe for that
   hardware, without restarting the ESP32. Confirm fresh measurements recover.
4. From valid operation, use a safe fault fixture to interrupt communication.
   Confirm no previous values appear in failed cycles and recovery occurs after
   restoration. Do not assume hot unplugging is safe for the selected module.
5. Confirm unset/invalid GPIO configuration produces one startup diagnostic and
   no measurement loop. Restore the intended configuration afterward.

These checks do not constitute the full product acceptance run: BLE and seven days
of unattended operation remain outside this increment's observed verification.

## References and dependency provenance

- Local [BME280 datasheet](../datasheets/bme280.pdf), BST-BME280-DS001-23,
  revision 1.23 (January 2022): sections 3.3/3.4 (modes/oversampling), 4.2 and
  appendix 8.1 (calibration/compensation), 5.4 (identity/reset/status/registers).
- [Bosch SensorAPI v3.5.1](https://github.com/boschsensortec/BME280_SensorAPI/tree/3ba9abb813660f900365b56e15cac5b7848bb6c4),
  commit `3ba9abb813660f900365b56e15cac5b7848bb6c4`; unmodified required source,
  headers and BSD-3-Clause license in `components/bosch_bme280/`.
- [ESP-IDF 6.1 master I2C API](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/i2c.html).

## Implementation verification record

See the change's [verification record](../../openspec/changes/add-bme280-serial-readings/verification.md)
for the recorded automated/build results.
Hardware checks remain unverified until run on confirmed hardware.
