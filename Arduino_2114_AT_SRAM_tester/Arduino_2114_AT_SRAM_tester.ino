/*
 * 2114_AT_SRAM_TESTER
 * (2114 UNO R3 Static RAM tester) 
 * by AdamT117 (kayto@github.com)
 * Use an Arduino UNO R3 to test 2114 SRAM ICs.
 * 
 * Pin Wiring Diagram (2114 SRAM to Arduino UNO R3):
 * I know its a pain but it really helps to record the 
 * wire colours!
 *
 * 2114 SRAM Pin Diagram (18-pin DIP, top-down view):
 * 
 *        +------------------+
 * A6     |1       +       18| Vcc (+5V, red)
 * A5     |2               17| A7 (D7, brown)
 * A4     |3               16| A8 (D8, grey)
 * A3     |4               15| A9 (D9, purple)
 * A0     |5    2114 SRAM  14| I/O1 (D13, orange)
 * A1     |6               13| I/O2 (D12, yellow)
 * A2     |7               12| I/O3 (D11, green)
 * /CE    |8               11| I/O4 (D10, blue)
 * GND    |9               10| /WE (A0/D14, orange)
 *        +------------------+
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
 * - Add *external* 10kΩ pull-down resistors from data pins (Arduino 10-13) to GND.
 *   i didn't bother but YMVV.
 * - Note delays implemented for 2114 SRAM timing: 50µs address setup, 500µs read cycle,
 *   250µs write pulse, 500µs total write cycle.
 * - Verbose logging: VERBOSE = true to log all writes and reads.
 * - Added start menu to choose full test or user tests.
 * - User tests to allow separate write/read operations with hex input.
 * - Added fill routine: Sets all memory to 0xF at the end of tests.
 * - Use 9600 baud and NEW LINE (select local echo if using seperate terminal emulator).
 * 
 * Original code inspiration from Carsten Skjerk June 2021.
 */
 // SPDX-License-Identifier: MIT
const byte addressPins[10] = {2, 15, 16, 3, 4, 5, 6, 7, 8, 9}; // A0-A9
const byte dataPins[4] = {10, 11, 12, 13}; // I/O4, I/O3, I/O2, I/O1
const byte WE = 14; // /WE (Write Enable, active low, Arduino A0)
#define VERBOSE false // Set to true for verbose logging

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
  delayMicroseconds(50); // Address setup time (20ns required, use 50µs)
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
    Serial.print(F("Write 0x"));
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
  delayMicroseconds(250); // Write pulse width (200ns required, use 250µs)
  digitalWrite(WE, HIGH);
  setDataBits(0);
  delayMicroseconds(200); // Total write cycle 500µs
}

byte readData(int address, bool verbose = false) {
  digitalWrite(WE, HIGH);
  setDataPinsInput();
  setAddressBits(address);
  delayMicroseconds(450); // Access time (450ns required, use 500µs total)
  byte result = 0;
  for (int i = 0; i < 4; i++) {
    bitWrite(result, 3 - i, digitalRead(dataPins[i]));
  }
  if (verbose) {
    Serial.print(F("Read 0x"));
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
        Serial.print(F("Error at 0x"));
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
      Serial.print(F("Alternating error 0x"));
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

void userTest() {
  Serial.println(F("\nUser Test"));
  Serial.println(F("Enter hex (e.g., 004, A)."));
  Serial.println(F("Select option:\n"));

  while (true) {
    Serial.println(F("Menu:"));
    Serial.println(F("1: Write (start addr, incr)"));
    Serial.println(F("2: Read range"));
    Serial.println(F("3: Fill memory with value"));
    Serial.println(F("4: Main menu"));
    Serial.print(F("Choice (1-4): ")); // Update prompt to reflect new option

    while (Serial.available() > 0) {
      Serial.read();
    }

    char choiceChar = readPrintableChar();
    if (choiceChar < '1' || choiceChar > '4') { // Update validation for new option
      Serial.println(F("Invalid choice! Use 1-4."));
      continue;
    }

    int choice = choiceChar - '0';

    if (choice == 4) {
      Serial.println();
      Serial.println(F("Fill mem. 0xF? (y/n)"));
      while (Serial.available() == 0);
      char confirm = readPrintableChar();
      if (confirm == 'y' || confirm == 'Y') {
        fillMemory(0xF);
      }
      Serial.println(F("To main menu..."));
      break;
    }

    if (choice == 1) {
      Serial.println(); // Add newline for clarity
      Serial.print(F("Start addr. (000-3FF): "));
      delay(10); // Give Serial buffer time to transmit
      while (Serial.available() > 0) Serial.read(); // Clear any leftover data
      char addressBuf[8];
      int i = 0;
      while (true) {
        while (Serial.available() == 0); // Wait for input
        char c = Serial.read();
        if (c == '\n' || c == '\r') break;
        if (i < 7) addressBuf[i++] = c;
      }
      addressBuf[i] = '\0';
      Serial.print(F("Addr: '"));
      Serial.print(addressBuf);
      Serial.println(F("'"));

      int address = parseHexInput(addressBuf, 8);

      if (address < 0 || address > 0x3FF) {
        Serial.println(F("Invalid addr.! Use 000-3FF."));
        continue;
      }

      Serial.println(F("Enter data (0-F)."));
      Serial.println(F("'q' to stop."));

      int currentAddress = address;
      while (currentAddress <= 0x3FF) {
        Serial.print(F("Addr 0x"));
        if (currentAddress < 0x10) Serial.print("0");
        if (currentAddress < 0x100) Serial.print("0");
        Serial.print(currentAddress, HEX);
        Serial.print(F(": Data (0-F, q): "));
        delay(10); // Give Serial buffer time to transmit
        while (Serial.available() > 0) Serial.read(); // Clear any leftover data
        char dataBuf[8];
        i = 0;
        while (true) {
          while (Serial.available() == 0);
          char c = Serial.read();
          if (c == '\n' || c == '\r') break;
          if (i < 7) dataBuf[i++] = c;
        }
        dataBuf[i] = '\0';

        if (dataBuf[0] == 'q' || dataBuf[0] == 'Q') {
          Serial.println(F("Stop write..."));
          break;
        }

        int data = parseHexInput(dataBuf, 8);

        if (data < 0 || data > 0xF) {
          Serial.println(F("Invalid data! Use 0-F."));
          continue;
        }

        Serial.print(F("Write 0x"));
        Serial.print((byte)data, HEX);
        Serial.print(F(" to 0x"));
        if (currentAddress < 0x10) Serial.print("0");
        if (currentAddress < 0x100) Serial.print("0");
        Serial.println(currentAddress, HEX);
        writeData(currentAddress, (byte)data, true);

        currentAddress++;
        delay(500);
      }

      if (currentAddress > 0x3FF) {
        Serial.println(F("Max addr. (0x3FF). Stop."));
      }
} else if (choice == 2) {
  Serial.println(); // Add newline for clarity
  Serial.print(F("Start addr (000-3FF): "));
  delay(10); // Give Serial buffer time to transmit
  while (Serial.available() > 0) Serial.read(); // Clear any leftover data
  char startAddressBuf[8];
  int i = 0;
  while (true) {
    while (Serial.available() == 0);
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    if (i < 7) startAddressBuf[i++] = c;
  }
  startAddressBuf[i] = '\0';
  Serial.print(F("Start: '"));
  Serial.print(startAddressBuf);
  Serial.println(F("'"));

  int startAddress = parseHexInput(startAddressBuf, 8);

  if (startAddress < 0 || startAddress > 0x3FF) {
    Serial.println(F("Invalid start! Use 000-3FF."));
    continue;
  }

  Serial.println(); // Add newline for clarity
  Serial.print(F("End addr ("));
  if (startAddress < 0x10) Serial.print("0");
  if (startAddress < 0x100) Serial.print("0");
  Serial.print(startAddress, HEX);
  Serial.print(F("-3FF): "));
  delay(10); // Give Serial buffer time to transmit
  while (Serial.available() > 0) Serial.read(); // Clear any leftover data
  char endAddressBuf[8];
  i = 0;
  while (true) {
    while (Serial.available() == 0);
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    if (i < 7) endAddressBuf[i++] = c;
  }
  endAddressBuf[i] = '\0';
  Serial.print(F("End: '"));
  Serial.print(endAddressBuf);
  Serial.println(F("'"));

  int endAddress = parseHexInput(endAddressBuf, 8);

  if (endAddress < startAddress || endAddress > 0x3FF) {
    Serial.print(F("Invalid end! Use "));
    if (startAddress < 0x10) Serial.print("0");
    if (startAddress < 0x100) Serial.print("0");
    Serial.print(startAddress, HEX);
    Serial.println(F("-3FF."));
    continue;
  }

  Serial.println();
  // Print single header row for 0x00 to 0x0F
  Serial.print(F("     ")); // Align with address column (e.g., "000: ")
  for (int offset = 0; offset < 16; offset++) {
    Serial.print(F(""));
    if (offset < 0x10) Serial.print("0");
    Serial.print(offset, HEX);
    Serial.print(" | ");
  }
  Serial.println();
 //               |    00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 0A | 0B | 0C | 0D | 0E | 0F | 
  Serial.println("--------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+");
  // Process addresses in chunks of 16
  int chunkSize = 16;
  // Start from the chunk containing startAddress
  int firstChunkStart = startAddress & ~0xF; // Round down to nearest multiple of 16
  for (int chunkStart = firstChunkStart; chunkStart <= endAddress; chunkStart += chunkSize) {
    int chunkEnd = min(chunkStart + chunkSize - 1, endAddress);

    // Skip chunks before startAddress or after endAddress
    if (chunkEnd < startAddress || chunkStart > endAddress) continue;

    // Print one data row for this chunk, labeled with chunkStart
    if (chunkStart <= endAddress && chunkEnd >= startAddress) {
      // Print chunk's base address
      if (chunkStart < 0x10) Serial.print("0");
      if (chunkStart < 0x100) Serial.print("0");
      Serial.print(chunkStart, HEX);
      Serial.print(F(": "));

      // Print data for addresses in this chunk
      for (int addr = chunkStart; addr <= chunkEnd; addr++) {
        if (addr >= startAddress && addr <= endAddress) {
          byte data = readData(addr, false);
          Serial.print(data, HEX); // Print data in hex (0-F)
          Serial.print("  | "); // Two spaces to align with header
        } else {
          Serial.print("    "); // Placeholder for alignment
        }
      }
      Serial.println();
    }
  }
    } else if (choice == 3) { // New option: Fill memory with user-defined value
      Serial.println(); // Add newline for clarity
      Serial.print(F("Fill val (0-F): "));
      delay(10); // Give Serial buffer time to transmit
      while (Serial.available() > 0) Serial.read(); // Clear any leftover data
      char fillBuf[8];
      int i = 0;
      while (true) {
        while (Serial.available() == 0); // Wait for input
        char c = Serial.read();
        if (c == '\n' || c == '\r') break;
        if (i < 7) fillBuf[i++] = c;
      }
      fillBuf[i] = '\0';
      Serial.print(F("Val: '"));
      Serial.print(fillBuf);
      Serial.println(F("'"));

      int fillValue = parseHexInput(fillBuf, 8);

      if (fillValue < 0 || fillValue > 0xF) {
        Serial.println(F("Invalid val! Use 0-F."));
        continue;
      }

      fillMemory((byte)fillValue);
    }

    Serial.println();
    delay(500);
  }
}

void startMenu() {
  while (true) {
    Serial.println(F("\n2114 SRAM Menu"));
    Serial.println(F("1: Full test"));
    Serial.println(F("2: User test"));
    Serial.println(F("3: Exit"));
    Serial.print(F("Choice (1-3): "));

    while (Serial.available() > 0) {
      Serial.read();
    }

    char choiceChar = readPrintableChar();
    if (choiceChar < '1' || choiceChar > '3') {
      Serial.println(F("Invalid choice! Use 1-3."));
      continue;
    }

    int choice = choiceChar - '0';

    if (choice == 1) {
      Serial.println(); // Add newline for clarity
      Serial.println(F("Start full test..."));
      runFullTest();
    } else if (choice == 2) {
      userTest();
    } else if (choice == 3) {
      Serial.println(); // Add newline for clarity
      Serial.println(F("Exit. Reset Arduino."));
      while (true);
    }
  }
}

void setup() {

  setupAddressPins();
  setDataPinsOutput();
  pinMode(WE, OUTPUT);
  digitalWrite(WE, HIGH);

  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println(F("2114 SRAM Tester by kayto@github.com"));
  Serial.println(F("Check SRAM..."));
  writeData(0, 0xA, true);
  setDataPinsInput();
  byte data = readData(0, true);
  if (data == 0x0 || data == 0xF) {
    Serial.println(F("Error: SRAM not detected."));
    while (1);
  } else {
    Serial.println(F("SRAM OK."));
  }

  startMenu();
}

void loop() {
  // Empty
}