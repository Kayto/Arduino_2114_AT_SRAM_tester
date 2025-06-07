#include <Arduino.h>

/*
 * 2114_AT_SRAM_TESTER (Automated)
 * (2114 UNO R3 Static RAM automated tester)
 * by AdamT117 (kayto@github.com)
 *
 * Use an Arduino UNO R3 to test 2114 SRAM ICs automatically on power-up/reset.
 *
 * Features:
 * - No user interaction required; all tests run automatically from setup().
 * - Tests include: all 4-bit patterns, alternating patterns, walking 1s/0s, and pseudo-random (LFSR) patterns.
 * - Pass/fail result is indicated on the external LED (A5):
 *     - Pass: slow blink 3x, then steady ON
 *     - Fail: fast blink forever
 * - Serial output for test progress and error reporting (optional, not required for operation).
 *
 * Pin Wiring Diagram (2114 SRAM to Arduino UNO R3):
 *
 * 2114 SRAM Pin Diagram (18-pin DIP, top-down view):
 *
 *                  +------------------+
 * (D6, red)     A6 |1       +       18| Vcc (+5V, red)
 * (D5, orange)  A5 |2               17| A7 (D7, brown)
 * (D4, yellow)  A4 |3               16| A8 (D8, grey)
 * (D3, green)   A3 |4               15| A9 (D9, purple)
 * (D2, grey)    A0 |5    2114 SRAM  14| I/O1 (D13, orange)
 * (A1, purple)  A1 |6               13| I/O2 (D12, yellow)
 * (A2, blue)    A2 |7               12| I/O3 (D11, green)
 * (GND, orange) /CE|8               11| I/O4 (D10, blue)
 * (GND, yellow) GND|9               10| /WE (A0/D14, orange)
 *                  +------------------+
 *
 *  2114 Pin | Function   | Connection
 *  ---------+------------+----------------------------------
 *     1     | A6         | Arduino digital pin 6 (red wire)
 *     2     | A5         | Arduino digital pin 5 (orange wire)
 *     3     | A4         | Arduino digital pin 4 (yellow wire)
 *     4     | A3         | Arduino digital pin 3 (green wire)
 *     5     | A0         | Arduino digital pin 2 (grey wire)
 *     6     | A1         | Arduino analog A1 (digital 15) (purple wire)
 *     7     | A2         | Arduino analog A2 (digital 16) (blue wire)
 *     8     | /CE (low)  | GND (orange wire)
 *     9     | GND        | GND (yellow wire)
 *  ---------+------------+----------------------------------
 *    18     | Vcc (+5V)  | +5V (red wire)
 *    17     | A7         | Arduino digital pin 7 (brown wire)
 *    16     | A8         | Arduino digital pin 8 (grey wire)
 *    15     | A9         | Arduino digital pin 9 (purple wire)
 *    14     | I/O1       | Arduino digital pin 13 (orange wire)
 *    13     | I/O2       | Arduino digital pin 12 (yellow wire)
 *    12     | I/O3       | Arduino digital pin 11 (green wire)
 *    11     | I/O4       | Arduino digital pin 10 (blue wire)
 *    10     | /WE (low)  | Arduino analog A0 (digital 14) (orange wire)
 *
 * Notes:
 * - /CE (pin 8) is tied to GND, keeping the chip always enabled.
 * - Add *external* 10kΩ pull-down resistors from data pins (Arduino 10-13) to GND for best results.
 * - Timing: Delays are implemented for 2114 SRAM timing (address setup, read, write pulse, write cycle).
 * - All tests and fill routines are fully automated; no serial input or menu system is present.
 * - Serial output is optional and for diagnostics only.
 * - LED on A5 is used for pass/fail indication (not the built-in LED).
 *
 * Original code inspiration from Carsten Skjerk June 2021.
 */
// SPDX-License-Identifier: MIT
const byte addressPins[10] = {2, 15, 16, 3, 4, 5, 6, 7, 8, 9}; // A0-A9
const byte dataPins[4] = {10, 11, 12, 13}; // I/O4, I/O3, I/O2, I/O1
const byte WE = 14; // /WE (Write Enable, active low, Arduino A0)
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

void logAddressBits(int address) {
  Serial.print(F("Addr. pins (A9-A0): "));
  for (int i = 9; i >= 0; i--) {
    Serial.print(digitalRead(addressPins[i]));
  }
  Serial.print(F(" (0x"));
  if (address < 0x10) Serial.print("0");
  if (address < 0x100) Serial.print("0");
  Serial.print(address, HEX);
  Serial.println(F(")"));
}

void setDataBits(byte value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(dataPins[i], bitRead(value, 3 - i)); // I/O4=bit 3, I/O1=bit 0
  }
}

void writeData(int address, byte data, bool verbose = false) {
  setAddressBits(address);
  setDataPinsOutput();
  digitalWrite(WE, LOW);
  setDataBits(data);
  if (verbose && VERBOSE) {
    Serial.print(F("  Write 0x"));
    if (address < 0x10) Serial.print("0");
    if (address < 0x100) Serial.print("0");
    Serial.print(address, HEX);
    Serial.print(F(": 0x"));
    Serial.print(data, HEX);
    Serial.print(F(" (I/O4-I/O1: "));
    for (int i = 0; i < 4; i++) {
      Serial.print(digitalRead(dataPins[i]));
      if (i < 3) Serial.print(",");
    }
    Serial.println(F(")"));
  }
  delayMicroseconds(2); // Write pulse width (200ns required)
  digitalWrite(WE, HIGH);
  setDataBits(0);
  delayMicroseconds(2); // Total write cycle 4µs
}

byte readData(int address, bool verbose = false) {
  digitalWrite(WE, HIGH);
  setDataPinsInput();
  setAddressBits(address);
  delayMicroseconds(5); // Access time (450ns required)
  byte result = 0;
  for (int i = 0; i < 4; i++) {
    bitWrite(result, 3 - i, digitalRead(dataPins[i]));
  }
  if (verbose) {
    Serial.print(F("  Read 0x"));
    if (address < 0x10) Serial.print("0");
    if (address < 0x100) Serial.print("0");
    Serial.print(address, HEX);
    Serial.print(F(": 0x"));
    Serial.print(result, HEX);
    Serial.print(F(" (I/O4-I/O1: "));
    for (int i = 0; i < 4; i++) {
      Serial.print(digitalRead(dataPins[i]));
      if (i < 3) Serial.print(",");
    }
    Serial.println(F(")"));
  }
  return result;
}

void printBinary(byte data) {
  for (int b = 3; b >= 0; b--) {
    Serial.print(bitRead(data, b));
  }
}

void fillMemory(byte value) {
  Serial.print(F("Fill mem. 0x"));
  Serial.println(value, HEX);
  for (int addr = 0; addr < 1024; addr++) {
    writeData(addr, value, false);
    if ((addr + 1) % 100 == 0) {
      Serial.print(".");
      if ((addr + 1) % 1000 == 0) Serial.println();
    }
  }
  Serial.println(F("\nFill done."));
}

void runFullTest() {
  int errorCount = 0;
  bool allSame = true;
  byte firstData = 0;

  for (byte pattern = 0; pattern < 16; pattern++) {
    Serial.print(F("Test pattern "));
    printBinary(pattern);
    Serial.print(" ");
    
    for (int addr = 0; addr < 1024; addr++) {
      writeData(addr, pattern, VERBOSE);
      byte data = readData(addr, VERBOSE);
      
      if (addr == 0 && pattern == 0) {
        firstData = data;
      } else if (data != firstData) {
        allSame = false;
      }
      
      if (data != pattern) {
        errorCount++;
        writeData(addr, pattern, true);
        byte reread = readData(addr, true);
        logAddressBits(addr);
        Serial.print(F("** Error at 0x"));
        if (addr < 0x10) Serial.print("0");
        if (addr < 0x100) Serial.print("0");
        Serial.print(addr, HEX);
        Serial.print(F(" - Got: "));
        printBinary(reread);
        Serial.print(F(" Exp: "));
        printBinary(pattern);
        Serial.println();
      }

      if ((addr + 1) % 100 == 0) {
        Serial.print(".");
        if ((addr + 1) % 1000 == 0) Serial.println();
      }
    }
    //Serial.println();
  }

  Serial.println(F("Test alternating patterns..."));
  for (int addr = 0; addr < 1024; addr++) {
    writeData(addr, (addr % 2) ? 0x5 : 0xA, VERBOSE);
    if ((addr + 1) % 100 == 0) {
      Serial.print(".");
      if ((addr + 1) % 1000 == 0) Serial.println();
    }
  }
  Serial.println();
  for (int addr = 0; addr < 1024; addr++) {
    byte expected = (addr % 2) ? 0x5 : 0xA;
    byte data = readData(addr, VERBOSE);
    if (data != expected) {
      errorCount++;
      writeData(addr, expected, true);
      byte reread = readData(addr, true);
      logAddressBits(addr);
      Serial.print(F("** Alternating error 0x"));
      if (addr < 0x10) Serial.print("0");
      if (addr < 0x100) Serial.print("0");
      Serial.print(addr, HEX);
      Serial.print(F(" - Got: "));
      printBinary(reread);
      Serial.print(F(" Exp: "));
      printBinary(expected);
      Serial.println();
    }
    if ((addr + 1) % 100 == 0) {
      Serial.print(".");
      if ((addr + 1) % 1000 == 0) Serial.println();
    }
  }
  Serial.println();

  if (allSame) {
    Serial.println(F("Warning: All same value. SRAM faulty?"));
  }
  
  Serial.print(F("Test done, "));
  Serial.print(errorCount);
  Serial.println(F(" errs."));

  fillMemory(0xF);
}

int parseHexInput(char* input, int maxLen) {
  int start = 0;
  int end = strlen(input) - 1;
  while (start <= end && (input[start] == ' ' || input[start] == '\r' || input[start] == '\n')) start++;
  while (end >= start && (input[end] == ' ' || input[end] == '\r' || input[end] == '\n')) end--;
  input[end + 1] = '\0';
  memmove(input, input + start, end - start + 2);

  char *endPtr;
  int value = strtol(input, &endPtr, 16);
  if (*endPtr != '\0') {
    Serial.print(F("Invalid hex: '"));
    Serial.print(input);
    Serial.println(F("'"));
    return -1;
  }
  return value;
}

char readPrintableChar() {
  char c;
  do {
    while (Serial.available() == 0);
    c = Serial.read();
    while (Serial.available() > 0) {
      Serial.read();
    }
  } while (c == '\n' || c == '\r');
  return c;
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
      if (data != (pattern & 0xF)) errorCount++;
    }
  }

  // 2. Alternating patterns (0xA/0x5, both 4-bit)
  for (int addr = 0; addr < 1024; addr++) {
    writeData(addr, ((addr % 2) ? 0x5 : 0xA) & 0xF, false);
  }
  for (int addr = 0; addr < 1024; addr++) {
    byte expected = ((addr % 2) ? 0x5 : 0xA) & 0xF;
    byte data = readData(addr, false) & 0xF;
    if (data != expected) errorCount++;
  }

  // 3. Walking 1s and 0s (4 bits only)
  for (byte bit = 0; bit < 4; bit++) {
    byte pattern = (1 << bit) & 0xF;
    for (int addr = 0; addr < 1024; addr++) writeData(addr, pattern, false);
    for (int addr = 0; addr < 1024; addr++) {
      byte data = readData(addr, false) & 0xF;
      if (data != pattern) errorCount++;
    }
    pattern = (~(1 << bit)) & 0xF;
    for (int addr = 0; addr < 1024; addr++) writeData(addr, pattern, false);
    for (int addr = 0; addr < 1024; addr++) {
      byte data = readData(addr, false) & 0xF;
      if (data != pattern) errorCount++;
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
    if (data != expected) errorCount++;
  }

  // 5. All same value check
  if (allSame) errorCount += 1024; // Force fail if all same

  // Fill memory with 0xF at end
  fillMemory(0xF);

  // LED indication
  if (errorCount == 0) {
    // Pass: blink LED slowly 3x, then ON
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH); delay(300);
      digitalWrite(LED_PIN, LOW); delay(300);
    }
    digitalWrite(LED_PIN, HIGH); // Steady ON
  } else {
    // Fail: fast blink LED forever
    while (1) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW); delay(100);
    }
  }
}

void setup() {
  setupAddressPins();
  setDataPinsOutput();
  pinMode(WE, OUTPUT);
  digitalWrite(WE, HIGH);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("2114 SRAM Automated Tester by kayto@github.com"));
  Serial.println(F("Check SRAM..."));
  writeData(0, 0xA, false);
  setDataPinsInput();
  byte data = readData(0, false);
  if (data == 0x0 || data == 0xF) {
    Serial.println(F("Error: SRAM not detected."));
    // Fail LED
    while (1) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW); delay(100);
    }
  } else {
    Serial.println(F("SRAM OK."));
  }

  automatedTest();
}

void loop() {
  // Empty, all tests run in setup()
}