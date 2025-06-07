#include <Arduino.h>

/*
 * 2114_AT_SRAM_TESTER (Automated, LED-only, Skjerk-compatible)
 * (2114 UNO R3 Static RAM automated tester)
 * by AdamT117 (kayto@github.com)
 *
 * Based on and fully compatible with Carsten Skjerk's original tester:
 *   https://github.com/skjerk/Arduino-2114-SRAM-tester
 *
 * CREDIT & ACKNOWLEDGEMENT:
 * This project and code are heavily inspired by and adapted from Carsten Skjerk's 2114 SRAM tester.
 *
 * IMPORTANT: THIS CODE IS FOR HEADLESS, LED-ONLY TESTING. IT IS NOT COMPATIBLE WITH SERIAL OR INTERACTIVE TESTS.
 * - This code uses Arduino digital pins D0 and D1 for address lines, which are also used for Serial RX/TX.
 * - As a result, you CANNOT use Serial communication, menus, or any interactive features with this code.
 * - This version is ONLY for automated, LED-indicated testing. Use it with Skjerk's original pin mapping and any hardware shield/PCB based on that reference.
 * - For interactive or serial-based tests, use a different version that does NOT use D0/D1 for address lines.
 *
 * Pin mapping and logic are FULLY compatible with Carsten Skjerk's original tester:
 *   https://github.com/skjerk/Arduino-2114-SRAM-tester/blob/master/Arduino_2114_SRAM_tester.ino
 *
 * === PIN MAPPING (Skjerk Reference, LED-Only Mode) ===
 *
 *  2114 Pin | Function   | Arduino Connection (UNO R3)
 *  ---------+------------+----------------------------------
 *     1     | A6         | D6 (red wire)
 *     2     | A5         | D5 (orange wire)
 *     3     | A4         | D4 (yellow wire)
 *     4     | A3         | D3 (green wire)
 *     5     | A0         | D0 (grey wire)   <== WARNING: D0 used, no Serial!
 *     6     | A1         | D1 (purple wire) <== WARNING: D1 used, no Serial!
 *     7     | A2         | D2 (blue wire)
 *     8     | /CE (low)  | GND (orange wire)
 *     9     | GND        | GND (yellow wire)
 *    10     | /WE        | A0 (analog 0, digital 14, orange wire)
 *    11     | I/O4       | D10 (blue wire)
 *    12     | I/O3       | D11 (green wire)
 *    13     | I/O2       | D12 (yellow wire)
 *    14     | I/O1       | D13 (orange wire)
 *    15     | A9         | D9 (purple wire)
 *    16     | A8         | D8 (grey wire)
 *    17     | A7         | D7 (brown wire)
 *    18     | Vcc (+5V)  | +5V (red wire)
 *
 *
 * FEATURES:
 * - Fully automated, no user interaction or menu system.
 * - Tests all 4-bit data patterns (0x0–0xF) at all 1024 addresses.
 * - Alternating bit patterns (0xA/0x5), walking 1s/0s, and pseudo-random (LFSR) patterns.
 * - Pass/fail result is indicated on the external LED (A5):
 *     - Pass: slow blink 3x, then steady ON
 *     - Fail: fast blink forever
 * - NO SERIAL OUTPUT OR DIAGNOSTICS DURING NORMAL OPERATION (HEADLESS MODE ONLY).
 */
// SPDX-License-Identifier: MIT
const byte addressPins[10] = {0,1,2,3,4,5,6,7,8,9}; // A0-A9
const byte dataPins[4] = {10,11,12,13}; // I/O4, I/O3, I/O2, I/O1
const byte RW = 14; // /WE (Write Enable, active low, Arduino A0)
#define VERBOSE false // Set to true for verbose logging
#define LED_PIN A5 // Use external LED on A5 for pass/fail indication (not LED_BUILTIN)

void setupAddressPins() {
  for (int i = 0; i < 10; i++) {
    pinMode(addressPins[i], OUTPUT);
  }
}

void setDataPinsOutput() {
  for (int i = 0; i < 4; i++) {
    pinMode(dataPins[i], OUTPUT);
  }
}

void setDataPinsInput() {
  for (int i = 0; i < 4; i++) {
    pinMode(dataPins[i], INPUT);
  }
}

void setAddressBits(int address) {
  for (int i = 0; i < 10; i++) {
    digitalWrite(addressPins[i], bitRead(address, i));
  }
  delayMicroseconds(2); // Address setup time (20ns required)
}

void setDataBits(byte value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(dataPins[i], bitRead(value, i)); // I/O4=bit 3, I/O1=bit 0
  }
}

void writeData(int address, byte data, bool verbose = false) {
  setAddressBits(address);
  setDataPinsOutput();
  digitalWrite(RW, LOW);
  setDataBits(data);
  delayMicroseconds(2); // Write pulse width (200ns required)
  digitalWrite(RW, HIGH);
  setDataBits(0);
  delayMicroseconds(2); // Total write cycle 4µs
}

byte readData(int address, bool verbose = false) {
  digitalWrite(RW, HIGH);
  setDataPinsInput();
  setAddressBits(address);
  delayMicroseconds(5); // Access time (450ns required)
  byte result = 0;
  for (int i = 0; i < 4; i++) {
    bitWrite(result, i, digitalRead(dataPins[i]));
  }
  return result;
}

void printBinary(byte data) {
  // Function retained for compatibility, but does nothing in headless mode
}

void fillMemory(byte value) {
  for (int addr = 0; addr < 1024; addr++) {
    writeData(addr, value, false);
  }
}

// Simple 10-bit LFSR for pseudo-random pattern generation

typedef struct {
  uint16_t state;
} LFSR10;

// Function prototypes for LFSR10
void lfsr10_init(LFSR10 *lfsr, uint16_t seed);
uint8_t lfsr10_next(LFSR10 *lfsr);

void lfsr10_init(LFSR10 *lfsr, uint16_t seed) {
  lfsr->state = seed ? seed : 0x1;
}

uint8_t lfsr10_next(LFSR10 *lfsr) {
  // taps: 10, 7 (bits 9, 6)
  bool new_bit = ((lfsr->state >> 9) ^ (lfsr->state >> 6)) & 1;
  lfsr->state = ((lfsr->state << 1) | new_bit) & 0x3FF;
  return lfsr->state & 0xF; // Only 4 bits for SRAM
}

// Automated test runner
void automatedTest() {
  int errorCount = 0;
  bool allSame = true;
  byte firstData = 0;

  // 1. All patterns 0x0 to 0xF (valid for 4-bit SRAM)
  for (byte pattern = 0; pattern < 16; pattern++) {
    for (int addr = 0; addr < 1024; addr++) {
      writeData(addr, pattern & 0xF, false); // Mask to 4 bits
    }
    for (int addr = 0; addr < 1024; addr++) {
      byte data = readData(addr, false) & 0xF; // Mask to 4 bits
      if (addr == 0 && pattern == 0) firstData = data;
      else if (data != firstData) allSame = false;
      if (data != (pattern & 0xF)) {
        errorCount++;
      }
    }
  }

  // 2. Alternating patterns (0xA/0x5, both 4-bit)
  for (int addr = 0; addr < 1024; addr++) {
    writeData(addr, ((addr % 2) ? 0x5 : 0xA) & 0xF, false);
  }
  for (int addr = 0; addr < 1024; addr++) {
    byte expected = ((addr % 2) ? 0x5 : 0xA) & 0xF;
    byte data = readData(addr, false) & 0xF;
    if (data != expected) {
      errorCount++;
    }
  }

  // 3. Walking 1s and 0s (4 bits only)
  for (byte bit = 0; bit < 4; bit++) {
    byte pattern = (1 << bit) & 0xF;
    for (int addr = 0; addr < 1024; addr++) writeData(addr, pattern, false);
    for (int addr = 0; addr < 1024; addr++) {
      byte data = readData(addr, false) & 0xF;
      if (data != pattern) {
        errorCount++;
      }
    }
    pattern = (~(1 << bit)) & 0xF;
    for (int addr = 0; addr < 1024; addr++) writeData(addr, pattern, false);
    for (int addr = 0; addr < 1024; addr++) {
      byte data = readData(addr, false) & 0xF;
      if (data != pattern) {
        errorCount++;
      }
    }
  }

  // 4. Pseudo-random pattern (LFSR, 4 bits only)
  LFSR10 lfsr;
  lfsr10_init(&lfsr, 0x1A3); // Arbitrary nonzero seed
  for (int addr = 0; addr < 1024; addr++) {
    byte pattern = lfsr10_next(&lfsr) & 0xF;
    writeData(addr, pattern, false);
  }
  lfsr10_init(&lfsr, 0x1A3); // Reset LFSR to same seed
  for (int addr = 0; addr < 1024; addr++) {
    byte expected = lfsr10_next(&lfsr) & 0xF;
    byte data = readData(addr, false) & 0xF;
    if (data != expected) {
      errorCount++;
    }
  }

  fillMemory(0xF);

  // LED indication
  if (errorCount == 0) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH); delay(300);
      digitalWrite(LED_PIN, LOW); delay(300);
    }
    digitalWrite(LED_PIN, HIGH); // Steady ON
  } else {
    while (1) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW); delay(100);
    }
  }
}

void setup() {
  setupAddressPins();
  setDataPinsOutput();
  pinMode(RW, OUTPUT);
  digitalWrite(RW, LOW); // Set READ mode
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  writeData(0, 0xA, false);
  setDataPinsInput();
  byte data = readData(0, false) & 0xF; // Mask to 4 bits for 2114 SRAM
  if (data == 0x0 || data == 0xF) {
    while (1) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW); delay(100);
    }
  }
  automatedTest();
}

void loop() {
  // Empty, all tests run in setup()
}