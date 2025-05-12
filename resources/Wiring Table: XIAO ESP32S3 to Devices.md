────────────────────────────────────────
Round Display → XIAO ESP32-S3
────────────────────────────────────────

| Display pad text | Connect to XIAO pad | Purpose |
|------------------|---------------------|---------|
| 5V | 5V | main 5 V rail |
| G | GND | ground |
| 3V | 3V3 | 3 V rail |
| 10 | — leave open — | (display MISO, unused) |
| 9 | D9 | SPI MOSI |
| 8 | D8 | SPI CLK |
| 7 | D7 | Touch INT |
| 6 | D6 | Back-light |
| 5 | D5 | I²C SCL (touch) |
| 4 | D4 | I²C SDA (touch) |
| 3 | D3 | Display DC |
| 2 | D2 | we will reuse as Encoder B |
| 1 | D1 | Display CS |
| 0 | D0 | we will reuse as Encoder A |

Result: 12 jumpers.
Pad 10 is intentionally left open so XIAO-D10 stays free for the NeoPixel ring.

────────────────────────────────────────
Rotary Encoder (no push-button)
────────────────────────────────────────

| Encoder pin | Connect to XIAO pad |
|-------------|---------------------|
| A | D0 |
| B | D2 |
| GND | GND |

(If your part has “SW”, ignore it.)
────────────────────────────────────────
NeoPixel 16-ring
────────────────────────────────────────
es
| Ring pad | Connect to | Note |
|----------|------------|------|
| DIN | D10 | (we freed D10 especially for this) |
| V+ | 5V | NeoPixels like 5 V |
| GND | GND | common ground |