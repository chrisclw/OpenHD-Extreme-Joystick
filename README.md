# OpenHD Extreme USB Joystick

A custom 8-channel USB HID joystick firmware for Teensy 3.2 and Teensy 4.0, designed for use with [OpenHD](https://openhdfpv.org/) FPV system over MAVLink RC control on a Raspberry Pi ground station.

## Features

- 8 analog RC channels via USB HID joystick
- 2 digital button inputs
- Compatible with Teensy 3.2 and Teensy 4.0
- Works with OpenHD over MAVLink (ArduPilot, iNAV, Pixhawk)
- Easy pin assignment configuration at the top of the sketch
- Supports continuous potentiometers, multi-position rotary switches, and toggle switches
- 6-position flight mode switch with ArduPilot PWM band mapping
- Auto-calculated switch position boundaries from ADC readings
- Serial Monitor debug output for wiring verification

---

## Hardware Requirements

- Teensy 3.2 or Teensy 4.0
- Arduino IDE with [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html) installed
- Potentiometers, rotary switches, and toggle switches for inputs
- Momentary pushbuttons for button inputs
- 3.3V power supply for all analog inputs (**never use 5V on Teensy analog pins**)

---

## RC Channel Mapping (Default)

| Channel | Function     | Pin | Input Type              |
|---------|-------------|-----|-------------------------|
| CH1     | Roll         | A0  | Continuous potentiometer |
| CH2     | Pitch        | A1  | Continuous potentiometer |
| CH3     | Throttle     | A2  | Continuous potentiometer |
| CH4     | Yaw          | A3  | Continuous potentiometer |
| CH5     | Flight Mode  | A7  | 6-position rotary switch |
| CH6     | Aux          | A5  | Continuous potentiometer |
| CH7     | Aux Slider   | A6  | Continuous potentiometer |
| CH8     | Aux Toggle   | A4  | 3-position toggle switch |
| BTN1    | Button 1     | 22  | Momentary switch to GND  |
| BTN2    | Button 2     | 23  | Momentary switch to GND  |

All pin assignments can be changed in the configuration section at the top of the sketch.

---

## Wiring

### Potentiometer (3 wires)
```
Pot Pin 1 (end)    → 3.3V
Pot Pin 2 (wiper)  → Analog pin (A0-A7)
Pot Pin 3 (end)    → GND
```

### Rotary / Toggle Switch (resistor ladder)
```
Switch output → Analog pin
               Measured via ADC (0-1023)
```

### Momentary Button (2 wires, no resistor needed)
```
Button Pin 1 → Digital pin (22 or 23)
Button Pin 2 → GND
```
INPUT_PULLUP is used internally — no external resistor required.

---

## Pre-Requisites — Required Core File Modifications

The standard Teensyduino joystick only supports 6 axes. To enable all 8 channels, two Teensyduino core files must be modified before compiling.

### 1. Enable Extreme Joystick Mode (JOYSTICK_SIZE 64)

Open `usb_desc.h` in your Teensy core folder:

| Board     | File Location |
|-----------|---------------|
| Teensy 3.2 | `<Arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy3/usb_desc.h` |
| Teensy 4.0 | `<Arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy4/usb_desc.h` |

Find the line under the `USB_FLIGHTSIM_JOYSTICK` section:
```c
#define JOYSTICK_SIZE 12
```
Change it to:
```c
#define JOYSTICK_SIZE 64
```

### 2. Fix CH8 (slider 2) Linux Compatibility

Open `usb_desc.c` in the same core folder. Find the joystick HID report descriptor section for `JOYSTICK_SIZE == 64`. Look for two consecutive lines:
```c
0x09, 0x36,    // Usage (Slider)
0x09, 0x36,    // Usage (Slider)
```
Change the **second** line from `0x36` to `0x37`:
```c
0x09, 0x36,    // Usage (Slider) — CH7 slider(1)
0x09, 0x37,    // Usage (Dial)   — CH8 slider(2)
```
This gives CH8's slider a unique HID usage code (Dial) so the Linux joystick driver on the Raspberry Pi recognizes it as a separate axis. Without this change, CH8 returns 65535 on Linux.

### 3. Arduino IDE Settings

```
Tools > Board    > Teensy 3.2  (or Teensy 4.0)
Tools > USB Type > Flight Sim Controls + Joystick
```

### 4. Full Recompile

After modifying any Teensy core files, hold **Shift** and click **Upload** in Arduino IDE to force a complete clean recompile. Then unplug and replug the Teensy from the Raspberry Pi to force USB re-enumeration.

---

## Configuration

All user settings are at the top of the sketch in the configuration section. You only need to edit this section — do not modify the code below it.

### Pin Assignments
```cpp
#define CH1_PIN   A0    // Roll
#define CH2_PIN   A1    // Pitch
// ... etc
#define BTN1_PIN  22
#define BTN2_PIN  23
```

### Switch Type
```cpp
#define CH5_IS_SWITCH  1   // 1 = switch, 0 = continuous pot
#define CH8_IS_SWITCH  1   // 1 = switch, 0 = continuous pot
```

### Flight Mode Switch Calibration (CH5)
Use the Serial Monitor debug output to find your switch's actual ADC readings, then enter them:
```cpp
#define CH5_POS1_ADC   0
#define CH5_POS2_ADC   205
#define CH5_POS3_ADC   255
// ... etc
```
Switch position boundaries are auto-calculated as midpoints between ADC readings — no manual calculation needed.

### Flight Mode PWM Values (CH5)
Default values map to ArduPilot flight mode PWM bands:

| Position | PWM Band    | Center | Joystick Value |
|----------|------------|--------|----------------|
| 1        | 0–1230     | ~1115  | 7536           |
| 2        | 1231–1360  | ~1295  | 19332          |
| 3        | 1361–1490  | ~1425  | 27852          |
| 4        | 1491–1620  | ~1555  | 36371          |
| 5        | 1621–1749  | ~1685  | 44891          |
| 6        | 1750+      | ~1875  | 57343          |

---

## Debugging

Uncomment the debug block at the bottom of `loop()` to print raw ADC values to Serial Monitor at 57600 baud:

```cpp
Serial.print("CH1:"); Serial.print(analogRead(CH1_PIN));
// ... etc
```

On startup, the Serial Monitor also prints your current channel assignments so you can verify the configuration is correct.

---

## Known Limitations

- **Maximum 8 analog axes** — the Linux kernel's joydev driver only recognizes 8 unique HID usage codes as joystick axes (X, Y, Z, Rx, Ry, Rz, Slider, Dial). A 9th analog axis cannot be added without changes to the Linux kernel or using a different input method.
- **Teensy 4.0 USB pinout** — the bottom pads on the Teensy 4.0 are USB Host only and cannot be used to connect to the Raspberry Pi. Use the Micro USB connector instead.
- **Teensy 3.2 USB pinout** — the bottom pads on the Teensy 3.2 are USB Device pads and can be used to connect to the Raspberry Pi directly, same as the Micro USB connector.
- **Teensyduino core file modifications are not persistent** — if you update Teensyduino, the `usb_desc.h` and `usb_desc.c` changes will be overwritten and must be reapplied.

---

## Tested With

- Teensy 3.2 + Raspberry Pi running OpenHD
- Teensy 4.0 + Raspberry Pi running OpenHD
- ArduPilot (MAVLink RC_CHANNELS_OVERRIDE)
- QOpenHD ground station

---

## License

MIT License — free to use, modify, and distribute with attribution.

---

## Acknowledgements

- [PJRC](https://www.pjrc.com/) — Teensy hardware and Teensyduino library
- [OpenHD](https://openhdfpv.org/) — open source FPV system
- [ArduPilot](https://ardupilot.org/) — open source flight controller firmware
