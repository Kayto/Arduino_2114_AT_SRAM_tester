# 2114_AT_SRAM_TESTER

An **Arduino UNO R3-based static RAM tester** for testing **2114 SRAM chips**.  
This tool allows you to verify the function of vintage 1K × 4-bit 2114 SRAM ICs through an automated full test or manual user-guided interface.

Created by [kayto@github.com](https://github.com/kayto)  
SPDX-License-Identifier: MIT

---

## Features

- Full automated memory test for all 1024 addresses (0x000–0x3FF)
- User test mode to read/write addresses or fill memory
- Detects chip presence at startup
- Verbose logging available via compile-time flag
- Hexadecimal input via Serial Monitor
- Memory filled with `0xF` at end of test

---

## Project Versions

### 1. Automated Version (`Arduino_2114_AT_SRAM_tester_auto.ino`)
- **Runs all tests automatically on power-up/reset.**
- No user interaction required.
- Pass/fail result is indicated on an external LED (A5):
  - Pass: slow blink 3×, then steady ON
  - Fail: fast blink forever
- Serial output for diagnostics and error reporting (optional).
- Use this version for quick, hands-off chip testing.

### 2. Interactive Version (`Arduino_2114_AT_SRAM_tester.ino`)
- **Interactive serial menu for user-driven tests and memory operations.**
- Full test: all 4-bit patterns, alternating patterns, error reporting.
- User test: manual write/read/fill operations with hex input.
- Fill routine: set all memory to a value (0xF default or user-defined).
- Serial output for test progress, error reporting, and data display.
- Use this version for detailed manual control and diagnostics.

---

## Requirements

- Arduino UNO R3
- 2114 SRAM chip (18-pin DIP)
- Jumper wires (color-coded recommended)
- (Optional) 4× 10kΩ pull-down resistors on data lines (D10–D13)
- **LED (plus 330Ω resistor) for pass/fail indicator (connect to A5 and GND)**

---

## 2114 to Arduino UNO R3 Wiring

```
2114 SRAM Pin Diagram (18-pin DIP, top-down view):

                         +------------------+
(D6 red)             A6  |1       +       18| Vcc    (+5V, red)
(D5 orange)          A5  |2               17| A7     (D7, brown)
(D4 yellow)          A4  |3               16| A8     (D8, grey)
(D3 green)           A3  |4               15| A9     (D9, purple)
(D2 grey)            A0  |5    2114 SRAM  14| I/O1   (D13, orange)
(A1 / D15 purple)    A1  |6               13| I/O2   (D12, yellow)
(A2 / D16 blue)      A2  |7               12| I/O3   (D11, green)
(GND)                /CE |8               11| I/O4   (D10, blue)
(GND)                GND |9               10| /WE    (A0/D14, orange)
                         +------------------+

(LED, A5)         LED INDICATOR: Connect LED (with series resistor, e.g. 330Ω) from A5 to GND
```

| 2114 Pin | Function   | Arduino Pin       |
|----------|------------|-------------------|
| 1        | A6         | D6 (red)          |
| 2        | A5         | D5 (orange)       |
| 3        | A4         | D4 (yellow)       |
| 4        | A3         | D3 (green)        |
| 5        | A0         | D2 (grey)         |
| 6        | A1         | A1 / D15 (purple) |
| 7        | A2         | A2 / D16 (blue)   |
| 8        | /CE        | GND               |
| 9        | GND        | GND               |
| 10       | /WE        | A0 / D14 (orange) |
| 11       | I/O4       | D10 (blue)        |
| 12       | I/O3       | D11 (green)       |
| 13       | I/O2       | D12 (yellow)      |
| 14       | I/O1       | D13 (orange)      |
| 15       | A9         | D9 (purple)       |
| 16       | A8         | D8 (grey)         |
| 17       | A7         | D7 (brown)        |
| 18       | Vcc        | +5V (red)         |
| -        | LED IND    | A5 → LED → GND    |

*Wire colors are suggestions to help organize connections.*

---

## Serial Monitor Usage (Interactive Version)

1. Upload the sketch to the Arduino UNO R3.
2. Open the Serial Monitor at **115200 baud**, line ending: **Newline**.
3. The startup routine checks for the presence of a 2114 SRAM chip.
4. You'll be presented with the following menu:

```
2114 SRAM Menu
1: Full test
2: User test
3: Exit
Choice (1-3):
```

### Option 1: Full Test
- Cycles all memory addresses with 16 data patterns (0x0 to 0xF)
- Verifies write-read integrity
- Tests alternating pattern memory (0xA / 0x5)
- Errors are reported by address and expected/actual value
- Memory is filled with 0xF at end of test

### Option 2: User Test

Provides manual control:
- **Write**: Specify start address and input values one-by-one
- **Read**: Read range of addresses (start–end)
- **Fill**: Fill all memory with a user-defined value

---

## Configuration

You can enable verbose output by editing this line in the sketch:

```cpp
#define VERBOSE true
```

This prints each write and read in detail, including data pin states and decoded values.

---

## Notes

- `/CE` is tied low (always enabled)
- `/WE` is driven by Arduino A0/D14
- Data bus is 4 bits wide (I/O1–I/O4 = D13–D10)
- Conservative timing delays are used for compatibility:
  - 50µs address setup
  - 250µs write pulse, 500µs total write cycle
  - 500µs read cycle

---

## License

This project is licensed under the MIT License.  
See the full license text in source code header.

---

## Acknowledgements

- Inspired by code from **Carsten Skjerk**, June 2021  
- Updated and expanded by [kayto@github.com](https://github.com/kayto)
