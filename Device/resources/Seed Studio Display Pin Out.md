# Seeed Studio Round Display (1.28") to Seeed Studio XIAO ESP32S3 Plus Wiring Map

This table maps the pins on the Seeed Studio Round Display header (designed for XIAO compatibility) to the corresponding pins on the **Seeed Studio XIAO ESP32S3 Plus** board, indicating which are used for the direct plug-in connection.

| Seeed Display Pin # | Function (on Display Board)        | Connects to XIAO Label | XIAO GPIO # | Notes                                      |
| :------------------ | :--------------------------------- | :--------------------- | :---------- | :----------------------------------------- |
| Pin 0               | Display Reset (RST)                | `D0`                   | 1           | Connected via Header                       |
| Pin 1               | Display Chip Select (CS)           | `D1`                   | 2           | Connected via Header                       |
| Pin 2               | *SD Card CS (Unused)*              | `D2`                   | 3           | **Available for External: Encoder A**      |
| Pin 3               | *Available/Unused*                 | `D3`                   | 4           | **Available for External: Encoder B**      |
| Pin 4               | I2C SDA                            | `D4`                   | 5           | Connected via Header                       |
| Pin 5               | I2C SCL                            | `D5`                   | 6           | Connected via Header                       |
| Pin 6               | Touch Interrupt (TP_INT)           | `D6`                   | 7           | Connected via Header                       |
| Pin 7               | Display Data/Command (DC)          | `D7`                   | 8           | Connected via Header                       |
| Pin 8               | SPI MOSI (Master Out, Slave In)    | `D8`                   | 9           | Connected via Header                       |
| Pin 9               | *SPI MISO (Unused)*                | `D9`                   | 10          | **Available for External: NeoPixel IN**    |
| Pin 10              | SPI Clock (SCK)                    | `D10`                  | 11          | Connected via Header                       |
| `3V3` Pad           | 3.3V Power Input                   | `3V3`                  | N/A         | Connected via Header                       |
| `GND` Pad           | Ground                             | `GND`                  | N/A         | Connected via Header                       |
| `VIN` Pad           | 5V Power Input                     | `5V`                   | N/A         | Connected via Header                       |
| `BL` Pad            | Backlight Control (Tie to 3.3V)    | `3V3`                  | N/A         | Connected via Header (if BL tied to 3V3) |

**External Peripheral Wiring:**

*   **Rotary Encoder A:** Connect to XIAO `D2` (GPIO3)
*   **Rotary Encoder B:** Connect to XIAO `D3` (GPIO4)
*   **NeoPixel Ring IN:** Connect to XIAO `D9` (GPIO8) (Note: 3.3V logic signal)
*   **Rotary Encoder GND:** Connect to XIAO `GND`
*   **NeoPixel Ring GND:** Connect to XIAO `GND`
*   **NeoPixel Ring V+:** Connect to XIAO `5V`

**Notes:**

*   The Round Display plugs directly onto the XIAO ESP32S3 Plus, connecting pins D0, D1, D4, D5, D6, D7, D8, D10 automatically.
*   Pins D2, D3, and D9 are used for external components (Encoder, NeoPixels).
*   Confirm the GPIO numbers when configuring firmware. 