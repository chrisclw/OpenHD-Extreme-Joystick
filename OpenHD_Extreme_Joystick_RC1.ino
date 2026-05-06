/*
 * ================================================================
 * OpenHD Extreme USB Joystick
 * ================================================================
 * Version:     RC1 (Release Candidate 1)
 * Platform:    Teensy 3.2 / Teensy 4.0
 * Purpose:     8-channel USB HID joystick for use with OpenHD
 *              over MAVLink on Raspberry Pi ground station
 * License:     MIT License
 *
 * Description:
 *   Converts analog inputs (potentiometers, rotary switches,
 *   toggle switches) and digital inputs (momentary buttons)
 *   into a USB HID joystick with 8 RC channels and 2 buttons.
 *   Designed for use with OpenHD FPV system over MAVLink RC.
 *
 * Supported Hardware:
 *   - Teensy 3.2 (analog pins A0-A7, A10-A11)
 *   - Teensy 4.0 (analog pins A0-A7)
 *
 * ================================================================
 * PRE-REQUISITES BEFORE COMPILING:
 *
 * 1. Edit usb_desc.h — change JOYSTICK_SIZE from 12 to 64:
 *      Teensy 3.2: <arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy3/usb_desc.h
 *      Teensy 4.0: <arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy4/usb_desc.h
 *
 * 2. Edit usb_desc.c — in joystick_report_desc for JOYSTICK_SIZE==64,
 *    change the 2nd slider usage code from 0x36 to 0x37 (Dial):
 *      Teensy 3.2: <arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy3/usb_desc.c
 *      Teensy 4.0: <arduino>/packages/teensy/hardware/avr/x.x.x/cores/teensy4/usb_desc.c
 *    Look for two consecutive lines with 0x09, 0x36 in the slider section.
 *    Change the SECOND one to 0x09, 0x37.
 *    This allows slider(2) to be recognized as a separate axis on Linux/Pi.
 *
 * 3. In Arduino IDE:
 *      Tools > Board    > Teensy 3.2 or Teensy 4.0
 *      Tools > USB Type > Flight Sim Controls + Joystick
 *
 * 4. Hold Shift + click Upload for a full clean recompile after
 *    any changes to the Teensy core files.
 *
 * WIRING:
 *   - Use 3.3V (NOT 5V) for all analog inputs on Teensy
 *   - Potentiometers: pin1=3.3V, pin2(wiper)=analog pin, pin3=GND
 *   - Buttons: one pin to digital pin, other pin to GND
 *     (INPUT_PULLUP is used — no external resistor needed)
 *
 * RC CHANNEL MAPPING (default):
 *   CH1 = Roll          (A0) — continuous potentiometer
 *   CH2 = Pitch         (A1) — continuous potentiometer
 *   CH3 = Throttle      (A2) — continuous potentiometer
 *   CH4 = Yaw           (A3) — continuous potentiometer
 *   CH5 = Flight Mode   (A7) — 6-position rotary switch
 *   CH6 = Aux           (A5) — continuous potentiometer
 *   CH7 = Aux Slider    (A6) — continuous potentiometer
 *   CH8 = Aux Toggle    (A4) — 3-position toggle switch
 *   BTN1 = pin 22 (A8)  — momentary switch to GND
 *   BTN2 = pin 23 (A9)  — momentary switch to GND
 *
 * NOTES:
 *   - Only 8 analog axes are supported due to Linux joydev driver
 *     limitations. The kernel only maps 8 unique HID usage codes
 *     to joystick axes (X, Y, Z, Rx, Ry, Rz, Slider, Dial).
 *   - slider(1) uses HID Usage Slider (0x36) — recognized on Pi
 *   - slider(2) uses HID Usage Dial   (0x37) — recognized on Pi
 *     after usb_desc.c modification described above
 * ================================================================
 */

// ================================================================
// USER CONFIGURATION — EDIT THIS SECTION TO MATCH YOUR WIRING
// ================================================================

// --- ANALOG PIN ASSIGNMENTS ---
// Assign the analog pin connected to each RC channel
// Available pins: A0-A7 (Teensy 3.2 also supports A10, A11)
#define CH1_PIN   A0    // Roll            — joystick left/right
#define CH2_PIN   A1    // Pitch           — joystick forward/back
#define CH3_PIN   A2    // Throttle        — potentiometer
#define CH4_PIN   A3    // Yaw             — joystick twist or pot
#define CH5_PIN   A7    // Flight Mode     — 6-position rotary switch
#define CH6_PIN   A5    // Aux             — potentiometer
#define CH7_PIN   A6    // Aux Slider      — potentiometer or switch
#define CH8_PIN   A4    // Aux Toggle      — 3-position toggle switch

// --- BUTTON PIN ASSIGNMENTS ---
// Momentary switches wired between pin and GND
// Set to -1 to disable a button
#define BTN1_PIN  22    // Momentary switch (A8 = digital pin 22)
#define BTN2_PIN  23    // Momentary switch (A9 = digital pin 23)

// --- SWITCH TYPE CONFIGURATION ---
// Set to 1 if channel uses a multi-position switch (not continuous pot)
// Set to 0 if channel uses a continuous potentiometer
#define CH5_IS_SWITCH  1   // 1 = switch, 0 = continuous pot
#define CH8_IS_SWITCH  1   // 1 = switch, 0 = continuous pot

// --- CH5 FLIGHT MODE SWITCH SETTINGS ---
// (only used when CH5_IS_SWITCH = 1)
//
// Step 1: Open Serial Monitor at 57600 baud and uncomment the
//         debug block at the bottom of loop() to read raw ADC values.
// Step 2: Note the ADC reading for each switch position and enter
//         them below. Set unused positions to the same value as
//         the last used position.
//
// ADC readings for each switch position (0-1023):
#define CH5_POS1_ADC   0      // Position 1 ADC reading
#define CH5_POS2_ADC   205    // Position 2 ADC reading
#define CH5_POS3_ADC   255    // Position 3 ADC reading
#define CH5_POS4_ADC   340    // Position 4 ADC reading
#define CH5_POS5_ADC   512    // Position 5 ADC reading
#define CH5_POS6_ADC   1023   // Position 6 ADC reading

// PWM output values for each switch position (16-bit, 0-65535)
// Default values map to ArduPilot flight mode PWM bands:
//   Flight Mode 1: PWM 0    - 1230  (center ~1115)
//   Flight Mode 2: PWM 1231 - 1360  (center ~1295)
//   Flight Mode 3: PWM 1361 - 1490  (center ~1425)
//   Flight Mode 4: PWM 1491 - 1620  (center ~1555)
//   Flight Mode 5: PWM 1621 - 1749  (center ~1685)
//   Flight Mode 6: PWM 1750+        (center ~1875)
// Formula: joystick_value = (PWM_center - 1000) / 1000 * 65535
#define CH5_POS1_PWM   7536   // Flight Mode 1 (PWM ~1115)
#define CH5_POS2_PWM   19332  // Flight Mode 2 (PWM ~1295)
#define CH5_POS3_PWM   27852  // Flight Mode 3 (PWM ~1425)
#define CH5_POS4_PWM   36371  // Flight Mode 4 (PWM ~1555)
#define CH5_POS5_PWM   44891  // Flight Mode 5 (PWM ~1685)
#define CH5_POS6_PWM   57343  // Flight Mode 6 (PWM ~1875)

// --- CH8 TOGGLE SWITCH SETTINGS ---
// (only used when CH8_IS_SWITCH = 1)
// ADC thresholds for 3-position toggle switch detection
#define CH8_LOW_THRESHOLD   50    // ADC below this  = LOW  position
#define CH8_HIGH_THRESHOLD  973   // ADC above this  = HIGH position
// Output values for each position (16-bit range 0-65535)
#define CH8_LOW_VAL    0      // LOW  position → PWM ~1000
#define CH8_MID_VAL    32767  // MID  position → PWM ~1500
#define CH8_HIGH_VAL   65535  // HIGH position → PWM ~2000

// ================================================================
// END OF USER CONFIGURATION — DO NOT EDIT BELOW THIS LINE
// ================================================================

// Scale 10-bit analogRead (0-1023) to 16-bit joystick range (0-65535)
#define AXIS_SCALE(x) ((x) * 64)

const int buttonPins[] = {BTN1_PIN, BTN2_PIN};
const int numButtons = 2;
byte allButtons[numButtons];
byte prevButtons[numButtons];

void setup() {
  Serial.begin(57600);
  Joystick.useManualSend(true);
  for (int i = 0; i < numButtons; i++) {
    if (buttonPins[i] >= 0) {
      pinMode(buttonPins[i], INPUT_PULLUP);
    }
  }
  Serial.println("=============================");
  Serial.println("OpenHD Extreme USB Joystick");
  Serial.println("RC1 — Teensy 3.2 / 4.0");
  Serial.println("=============================");
  Serial.println("Channel assignments:");
  Serial.print("  CH1 (Roll)        = A"); Serial.println(CH1_PIN - A0);
  Serial.print("  CH2 (Pitch)       = A"); Serial.println(CH2_PIN - A0);
  Serial.print("  CH3 (Throttle)    = A"); Serial.println(CH3_PIN - A0);
  Serial.print("  CH4 (Yaw)         = A"); Serial.println(CH4_PIN - A0);
  Serial.print("  CH5 (Flight Mode) = A"); Serial.println(CH5_PIN - A0);
  Serial.print("  CH6 (Aux)         = A"); Serial.println(CH6_PIN - A0);
  Serial.print("  CH7 (Aux Slider)  = A"); Serial.println(CH7_PIN - A0);
  Serial.print("  CH8 (Aux Toggle)  = A"); Serial.println(CH8_PIN - A0);
  Serial.print("  BTN1              = pin "); Serial.println(BTN1_PIN);
  Serial.print("  BTN2              = pin "); Serial.println(BTN2_PIN);
  Serial.println("=============================");
}

// Map 6-position switch ADC reading to joystick output value
// Boundaries are auto-calculated as midpoints between ADC readings
int mapSwitch6(int raw) {
  int b1 = (CH5_POS1_ADC + CH5_POS2_ADC) / 2;
  int b2 = (CH5_POS2_ADC + CH5_POS3_ADC) / 2;
  int b3 = (CH5_POS3_ADC + CH5_POS4_ADC) / 2;
  int b4 = (CH5_POS4_ADC + CH5_POS5_ADC) / 2;
  int b5 = (CH5_POS5_ADC + CH5_POS6_ADC) / 2;
  if      (raw < b1) return CH5_POS1_PWM;
  else if (raw < b2) return CH5_POS2_PWM;
  else if (raw < b3) return CH5_POS3_PWM;
  else if (raw < b4) return CH5_POS4_PWM;
  else if (raw < b5) return CH5_POS5_PWM;
  else               return CH5_POS6_PWM;
}

// Map 3-position toggle switch ADC reading to joystick output value
int mapToggle3(int raw) {
  if      (raw < CH8_LOW_THRESHOLD)  return CH8_LOW_VAL;
  else if (raw > CH8_HIGH_THRESHOLD) return CH8_HIGH_VAL;
  else                               return CH8_MID_VAL;
}

void loop() {

  // CH1: Roll — X axis
  Joystick.X(AXIS_SCALE(analogRead(CH1_PIN)));

  // CH2: Pitch — Y axis
  Joystick.Y(AXIS_SCALE(analogRead(CH2_PIN)));

  // CH3: Throttle — Z axis
  Joystick.Z(AXIS_SCALE(analogRead(CH3_PIN)));

  // CH4: Yaw — Xrotate
  Joystick.Xrotate(AXIS_SCALE(analogRead(CH4_PIN)));

  // CH5: Flight Mode — Yrotate (switch or continuous pot)
  #if CH5_IS_SWITCH
    Joystick.Yrotate(mapSwitch6(analogRead(CH5_PIN)));
  #else
    Joystick.Yrotate(AXIS_SCALE(analogRead(CH5_PIN)));
  #endif

  // CH6: Aux — Zrotate
  Joystick.Zrotate(AXIS_SCALE(analogRead(CH6_PIN)));

  // CH7: Aux Slider — slider(1)
  // Uses HID Usage Slider (0x36) — recognized by Linux Pi driver
  Joystick.slider(1, AXIS_SCALE(analogRead(CH7_PIN)));

  // CH8: Aux Toggle — slider(2)
  // Uses HID Usage Dial (0x37) — recognized by Linux Pi driver
  // Requires usb_desc.c modification (see pre-requisites above)
  #if CH8_IS_SWITCH
    Joystick.slider(2, mapToggle3(analogRead(CH8_PIN)));
  #else
    Joystick.slider(2, AXIS_SCALE(analogRead(CH8_PIN)));
  #endif

  // Buttons — LOW = pressed (INPUT_PULLUP)
  for (int i = 0; i < numButtons; i++) {
    if (buttonPins[i] >= 0) {
      allButtons[i] = !digitalRead(buttonPins[i]);
    } else {
      allButtons[i] = 0;
    }
    Joystick.button(i + 1, allButtons[i]);
  }

  // Send all axis and button data to host
  Joystick.send_now();

  // --- Debug: uncomment to print raw ADC values to Serial Monitor ---
  // Use this to find ADC readings for switch positions (CH5, CH8)
  /*
  Serial.print("CH1:"); Serial.print(analogRead(CH1_PIN));
  Serial.print(" CH2:"); Serial.print(analogRead(CH2_PIN));
  Serial.print(" CH3:"); Serial.print(analogRead(CH3_PIN));
  Serial.print(" CH4:"); Serial.print(analogRead(CH4_PIN));
  Serial.print(" CH5:"); Serial.print(analogRead(CH5_PIN));
  Serial.print(" CH6:"); Serial.print(analogRead(CH6_PIN));
  Serial.print(" CH7:"); Serial.print(analogRead(CH7_PIN));
  Serial.print(" CH8:"); Serial.print(analogRead(CH8_PIN));
  Serial.println();
  */

  // Print button state changes to Serial Monitor
  boolean anyChange = false;
  for (int i = 0; i < numButtons; i++) {
    if (allButtons[i] != prevButtons[i]) anyChange = true;
    prevButtons[i] = allButtons[i];
  }
  if (anyChange) {
    Serial.print("Buttons: ");
    for (int i = 0; i < numButtons; i++) {
      Serial.print(allButtons[i], DEC);
      Serial.print(" ");
    }
    Serial.println();
  }

  delay(10);
}
