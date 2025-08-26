# Resideo WV8840C Modbus Protocol

The Resideo WV8840C water heater control has a port labeled COM. This port is
used with the Bradford White ICON Acessory Module Kit (239-47872-00). It
includes a setback control and leak detection capability. It also has a cold
water mixing feature which allows the tank temperature to be maintained higher
than the outlet temperature giving greater capacity.

This document is written based on testing done with a Resideo WV8840C module.
However, there is a wide range of similar modules sold under different brands
(Honeywell, Bradford White, etc) and model numbers (WT8840, WT8860, etc).

## Physical/Electrical Interface

The interface is a 3.3V level serial connection. It utilizes IRDA signalling
at 2400 baud with the RX line inverted (idle high). When the serial connection
is used, the device logic is powered by the RX line through a diode and 15
ohm resistor. A 220uF capacitor is present, so inrush currents can be as high
as 220mA.

The WV8840 and similar modules use a Lumberg RAST 2.5 3512 03 connector. Other
RAST 2.5 or even generic card edge connectors may work, such as the the TE
1-2391423-3. WV4460 and similar modules use a different (unknown) connector.
The pins are labeled on the PCB from left to right RX (pin 1), TX, GND. The
direction indicated is from the MSP430 on the PCB.

## Modbus Commmunications Interface

The communications protocol is Modbus RTU over serial line with the water
heater control acting as a slave. All temperatures are stored in tenths of a
degree Farenheight. Because the MSP430 is a little-endian device and Modbus is
a big-endian protocol, all byte pairs transmitted are swapped before being
written to the serial buffer and all received byte pairs are swapped before
being processed. The server ID string is an exception to this byte swapping.
The address byte is ignored. The maximum return buffer size is 26 bytes.

4 Modbus function codes are supported:

| Function name                    | Function code | Note                               |
|----------------------------------|---------------|------------------------------------|
| Read multiple holding registers  | 0x03          | Equivalent to 0x04                 |
| Read input registers             | 0x04          | Equivalent to 0x03                 |
| Write single holding register    | 0x06          |                                    |
| Write multiple holding registers | 0x10          |                                    |
| Report server ID                 | 0x11          | ASCII Model named followed by 0xfe |

Exception codes returned are shown below.

| Exception Text                   | Exception code | Note                      |
|----------------------------------|----------------|---------------------------|
| Illegal function                 | 0x01           |                           |
| Illegal data address             | 0x02           |                           |
| Illegal data value               | 0x03           | Read count too large or 0 |
| Server device failure            | 0x04           | Model name does not fit within the return buffer |

## Modbus Register Banks

The Modbus registers are segmented into banks. The high address byte indicates
the bank number and the low address byte indicates the register within the bank.
The banks are listed below:

ID | RO | Size (words) | Name
---|----|--------------|-----
0  | RO | 15           | State variables
1  | RW | 3            | Control variables
2  | RW | 6            | Command
3  | RO | 8            | Product information
4  | RO | 128          | Device information block
5  | RO | 6            | Unknown
6  | RO | 21           | Problem information
7  | RO | 21           | Problem mapping

## State Variables (Bank 0)

The state variable bank contains the current state of the control unit. The
registers are in the table below:

ID | Units    | Note | Description
---|----------|------|------------
0  | VCC/4095 | 1    | NTC detect voltage (right sensor)
1  | VCC/4095 | 1    | NTC detect voltage (left sensor)
2  | Tenths F |      | NTC sensor raw temperature
3  | Tenths F |      | Estimated tank temperature
4  | Tenths F |      | MCU (ambient) temperature
5  | Tenths F | 2    | Temperature setting of dial
6  |          | 3    | Heating and manifold status word
7  |          | 4    | Current problems
8  |          |      | Unused
9  |          |      | Solenoid drive voltage
10 | Tenths F | 2    | Temperature set-point (includes setback)
11 | Tenths F |      | Temperature hysteresis
12 | Tenths F | 2    | Last temperature set-point
13 | Tenths F |      | Last temperature hysteresis
14 |          |      | Unused

1. The sense line for the NTC is between a 2.1k resistor driven to VCC and
the NTC tied to ground. To determine the resistance of the NTC, use the
following:

    x * 2100.0 / (4095.01 - x);

If the value read is less than 201 or more than 4024, it is considered invalid
and not stored in the register. If no valid value has been read since
power-up, the register will contain the value 4025.

2. The temperature setting value is 0 (0.0 F) if the control is in the OFF
position and 1 (0.1 F) if the control is in the PILOT position.

3. Bits 5-0 are set if the system is currently in heating mode (gas manifold
commanded open). Bits 21-16 are set if the gas manifold mosfet is currently
being driven. Note that this signal is a PWM so when it is active it will
randomly read as low vs high. Keep in mind that these will be received byte
swapped so that the first received byte will contain the gas manifold mosfet
status.

4. This contains a bitfield indicating current problem status. Problem
definitions likely vary by model, but a mapping between a problem bit position
and the number of associated LED flashes exists in the accessible information
block. While the meaning of LED flash counts also varies between models, this
is at least available in public documentation. See the Problems section for
more information.

## Control Variables (Bank 1)

ID | Units    | Note | Description
---|----------|------|------------
0  | Tenths F | 1    | Setback temperature
1  | Tenths F | 1    | Temperature hysteresis override
2  |          | 2    | Inject problem

1. The temperature override settings are ignored if the value is 0x7fff. Bank
3, product information, cotains the minimum/maximum limits of these value.

2. For the WV8840C the only accepted problem flag is 0x0040, tank leakage
detected.

## Factory Control (Bank 2)

This bank contains registers used for factory calibration and diagnostics.

ID | Description
---|------------
0  | Command
1  | Argument
2  | Auto readback 1 start
3  | Auto readback 1 count
4  | Auto readback 2 start
5  | Auto readback 2 start

### Factory Control Commands

A system is present to allow remote control of various internal functions. A
command can be written to the command and argument can be written to the
command/argument register pair and then the status monitored by reading back
the command register. While running, the high byte of the command word
indicates status. It should be written as zero when initiating a command.

Status | Note | Description
-------|------|------------
0x01   |      | Command running
0x02   |      | Command completed successfully
0x06   | 1    | Invalid command
0x07   | 2    | Invalid command

1. This response is given to an invalid command given during normal operation.
2. This response is given to an invalid command given while the system is
in factory calibration mode.

Many commands have specific status codes that are documented along with it's
associated modbus command.

Commands associated with normal operation are shown below:

Command | Note | Description
--------|------|------------
0xaa    | 1    | Serial connection active
0xf0    |      | Enter factory calibration mode
0xf2    | 2    | System reset complete
0xf3    |      | Check for problems

1. Automatically initiated when RX line is detected as being pulled high. This
modifies the VCC management routines as VCC is now provided by the remote
serial host.

2. This command cannot be given during normal operation, but is the default
contents of the command register at reset, along with the 0x02 status byte.

#### Check for Problems (Command 0xf3)

This command completes successfully (status code 0x02) if no problems are
present, but indicates status code 0x04 if one or more problems are present.

Commands associated with factory calibration mode:

Command | Argument  | Description
--------|-----------|------------
0x00    | Target    | Calibrate ambient temperature
0x01    | Target    | Calibrate potentiometer pilot position
0x02    | Target    | Calibrate potentiometer hot position
0xf1    |           | Write calibration values
0xf2    |           | System reset (Exit factory calibration mode)
0xf4    | 0x045a    | Erase calibration values
0xf5    | Region ID | Erase information block region
0xf6    |           | Enable red LED and disable auto-transformer boost
0xf7    |           | Enable red LED and enable auto-transformer boost
0xf8    |           | Disable red LED and disable auto-transformer boost
0xf9    |           | Disable red LED and enable auto-transformer boost


#### Calibrate Ambient Temperature (Command 0x00)

This command calculates a required temperature offset value for the MSP430
temperature sensor. The argument should be the current ambient temperature in
tenths F. Additional status codes are below:

ID   | Description
-----|------------
0x03 | Actual varies more than max delta (50) during measurement
0x04 | Actual outside min/max boundaries (4950/6150)
0x08 | Target outside accepted range (47.0 F/107.0 F)

When the command completes with a 0x02 status byte, the raw MSP430 temperature
reading can be read back from the argument register.

### Calibrate Potentiometer Pilot Position (Command 0x01)

This command is used to calibrate the pilot position of the potentiometer
knob. The argument should be set to the appropriate calibration target value
for this step, 1920. The knob should be moved to the pilot position before
issuing the command. Additional status codes are below:

ID   | Description
-----|------------
0x03 | Actual varies more than max delta (7) during measurement
0x04 | Actual outside min/max boundaries (310/910)
0x08 | Target outside accepted range (1280/2560)

When the command completes with a 0x02 status byte, the raw MSP430 ADC reading
can be read back from the argument register.

#### Calibrate Potentiometer Hot Position (Command 0x02)

This command is used to calibrate the "Hot" position of the potentiometer
knob. The argument should be set to the appropriate calibration target value
for this step, 8000. The knob should be moved to the "Hot" position before
issuing the command. Additional status codes are below:

ID   | Description
-----|------------
0x03 | Actual varies more than max delta (7) during measurement
0x04 | Actual outside min/max boundaries (1625/2345)
0x08 | Target outside accepted range (7360/8640)

When the command completes with a 0x02 status byte, the raw MSP430 ADC reading
can be read back from the argument register.

### Store Current Calibration (Command 0xf1)

This command stores the data from the above calibration steps in the
information block. Additional status codes are below:

ID   | Description
-----|------------
0x05 | Flash command failed
0x06 | Calibration steps not completed

#### System Reset (Command 0xf2)

This command exits factory calibration mode by performing a system reset.

#### Erase Calibration Data (Command 0xf4)

This command erases any currently stored calibration data (part of Segment C).
It also verifies that the calibration bytes (0x10fe/0x10ff) for 1MHz operation
are present (not 0x00 or 0xff). The argument word must be set to 0x045a.
Additional status codes are below:

ID   | Description
-----|------------
0x04 | Argument word not set correctly
0x05 | Flash command failed or calibration bytes not present

#### Clear Problem Log (Command 0xf5)

This command erases Segment D (0x1000-0x103f) or Segment B (0x10080-0x10bf)
depending on the argument value. An argument value of 0x015a will erase
Segment D which contains the major problem log. An argument value of 0x025a
will erase Segment B which contains the minor problem log as well as the
current heating cycle count. Additional status codes are below:

ID   | Description
-----|------------
0x04 | Argument word not set correctly
0x05 | Flash command failed

#### Cycle Power Controls (Commands 0xf6-0xf9)

The purprose of these are a bit unclear, but they control the state of a
mosfet connecting the relay voltage rail and the thermocouple input as well
as the red LED

ID   | Red LED  | Mosfet
-----|----------|-------
0xf6 | Enabled  | Disabled
0xf7 | Enabled  | Enabled
0xf8 | Disabled | Disabled
0xf9 | Disabled | Enabled

### Auto Readback Registers

Two sets of registers can be periodically read back to the host. All four
auto readback registers must be programmed with valid register ranges. The
returned function code is 0x44. The byte immediately after the function code
is data length, eg, all bytes following this byte excluding the CRC. The
next byte indicates how many bytes are present in the first register set
followed by the first register set, then the number of bytes in the second
register set followed by the second register set. The message including the
function code and data length byte must fit within 26 bytes.

## Product Info (Bank 3)

This register bank contains basic product information.

ID | Units    | Note | Description
---|----------|------|------------
0  |          | 1    | Model name bytes 0/1
1  |          | 1    | Model name bytes 2/3
2  |          | 1    | Model name bytes 4/5
3  |          | 1    | Model name bytes 6/7
4  | Tenths F |      | Maximum setback temperature
5  | Tenths F |      | Minimum setback temperature
6  | Tenths F |      | Maximum temperature hysteresis
7  | Tenths F |      | Minimum temperature hysteresis

1. Model name bytes are swapped and string is right padded with spaces.

## Information Block (Bank 4)

This bank simply contains the full contents of the memory information block.
Rather than documenting these by register ID, they are documented here by
address within the MSP430 memory map. When reading these vias Modbus, 0x1000
should be subtracted from the address and it should be divided by 2.

As described above, all data is returned byte swapped.

Address | Segment | Size | Description
--------|---------|------|------------
0x1000  | D       | 0x40 | Stored major problems
0x1040  | C       | 0x10 | Unknown
0x1050  | C       | 0x10 | Primary calibration block
0x1060  | C       | 0x10 | Backup calibration block
0x1070  | C       | 0x05 | Blank
0x1075  | C       | 0x09 | Stepped array for monitoring pilot start
0x107e  | C       | 0x02 | Backup DCO 1MHz calibration value
0x1080  | B       | 0x10 | Stored minor problems
0x1090  | B       | 0x2e | Heating cycle count (short value)
0x10be  | B       | 0x02 | Heating cycle count (long value)
0x10c0  | A       | 0x38 | Blank
0x10f8  | A       | 0x02 | DCO 16MHZ calibration value
0x10fa  | A       | 0x02 | DCO 12MHZ calibration value
0x10fc  | A       | 0x02 | DCO 8MHZ calibration value
0x10fe  | A       | 0x02 | DCO 1MHZ calibration value

#### Stored Major Problems (0x1000)

This data stores a log of major problems. Major problems immediately put the
system into a quiesscent state until it is power cycled. Note that when this
occurs, it is not even reachable via Modbus. For the WV8840C major problems
include the problem flags 0x001e, or:

* TCO exceeded
* Potentiometer failure
* Memory corruption
* Solenoid voltage discharge problem
* Solenoid voltage drive problem

Each entry in the log is 4 bytes, with the lower byte being a or'd bitmask of
the current major problems and the upper 3 bytes being the heating cycle
count.

When the log is filled (16 entries, 64 bytes) the flash segment is erased and
the newest 10 entries are rewritten before writing the newest entry.

When the system starts, it checks this log and if there is an entry that
matches the current heating cycle, the error is used to flash the error LED
the proper count until the error is verified as cleared.

#### Unknown (0x1040)

The purpose of these bytes are unknown and unused by the firmware, but given
their location they likely have to do with factory calibration procedure.

#### Primary calibration block (0x1050)

This is the data written by the factory calibration routine. It is used to
read a calibration potentiometer position as well as the MSP430 temperature
reading.

Offset | Size | Description
-------|------|------------
0x00   | 0x02 | Unused
0x02   | 0x02 | Potentiometer pilot set point
0x04   | 0x02 | Potentiometer "Low" set point
0x06   | 0x02 | Potentiometer "Hot" set point
0x08   | 0x02 | Potentiometer "Very Hot" set point
0x0a   | 0x02 | MSP430 temperature offset
0x0c   | 0x02 | Solenoid detect voltage offset (unused)
0x0e   | 0x01 | unused
0x0f   | 0x01 | 8-bit summation checksum

#### Backup Calibration Block (0x1060)

The backup calibration block is written at the same time as the primary
block and is used if the checksum fails.

#### Stepped Array for Monitoring Pilot Start (0x1075)

The purpose of these bytes are a bit unclear, but if the serial port is
connected (RX pulled high), then these bytes are cleared to zero is a step
wise manner, odd for pilot, even for manifold if certain conditions are met.

#### Stored Minor Problems (0x1080)

This stores minor (not major) problem events. Each array element is 2 bytes.
The array can contain up to 8 elements (16 bytes) and is only written to until
it becomes full. It can only be reset within factory calibration mode. The
first byte is the problem index and the second byte is the short portion of
the heating cycle count.

#### Heating Cycle Count (0x1090)

This region tracks the number of heating cycles. It is incremented each time
the gas manifold is opened. The first 46 bytes are used to track the short
portion of the heating cycle. Each byte is cleared to 0x00 in sequence. Once
all 46 bytes have been cleared, the entire segment is erased and the long
cycle count is increment and written. The number of heating cycles is thus:

    short cycle count + (long cycle count + 1) * 47


## Empty/Unknown (Bank 5)

ID | Description
---|------------
0  | 0x0000
1  | 0x0000
2  | 0x7fff
3  | 0x7fff
4  | 0x0000
5  | 0x0000

## Problem Information (Bank 6/7)

These two banks contains information associated with problems that can occur.
The register ID equates to the internal problem idx. The purpose of the first
bank is unknown, but the second bank allows a problem idx to be translated to
a problem flag.

ID (idx) | Bank 6 | Bank 7 (flag) | Description
---------|--------|---------------|------------
0        | 0x0001 | 0x0001        | Turning off
1        | 0x0007 | 0x0002        | Temperature cutoff exceeded
2        | 0x0000 | 0x0004        | Potentiometer failure
3        | 0xc000 | 0x0004        | Memory corruption
4        | 0xc000 | 0x0008        | Solenoid voltage discharge problem
5        | 0x005d | 0x0010        | Solenoid voltage drive problem
6        | 0x0059 | 0x0020        | Unused
7        | 0x0059 | 0x0040        | Tank leakage reported
8        | 0x0059 | 0x0080        | Solenoid voltage level problem
9        | 0x0059 | 0x0100        | Manifold solenoid problem
10       | 0x009b | 0x0200        | Potentiometer driver problem
11       | 0x008f | 0x0400        | Information block corruption
12       | 0x0012 | 0x0800        | Right NTC sensor problem
13       | 0x0012 | 0x0800        | Left/Right NTC sensor mismatch
14       | 0x0012 | 0x0800        | Left NTC sensor problem
15       | 0x0012 | 0x0800        | NTC sensor problem
16       | 0x0020 | 0x1000        | Unused
17       | 0x0012 | 0x2000        | Unused
18       | 0x009c | 0x4000        | Unused
19       | 0x0096 | 0x8000        | Low thermopile voltage
20       | 0x0012 | 0x0200        | Unused

Problems 1-5 (flags 0x001e) are considered to be "major".

The following table maps problem flags to flash counts, it unfortunately is
not available via Modbus:

Flash count | Flags  | Description
------------|--------|------------
2           | 0x8000 | Low thermopile voltage; main valve not turned ON
3           | 0x0000 | Insufficient water heating; water temperature not rising when it should
4           | 0x0002 | Temperature cut-out limit reached
5           | 0x0800 | Water temperature sensor failure
6           | 0x0040 | Tank leakage detected by accessory module
7           | 0x179c | Electronics failure
8           | 0x0001 | This is just a warning: The control does not see power decaying with the knob in the off position
9           | 0x0000 | Door temperature sensor failure
10          | 0x0000 | Abnormal combustion chamber temperature profile during heating cycle

Entries with an empty flags value indicate that the flash count will not be
seen on this model.
