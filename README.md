# Ping Pong Scoring System

ESP32-based scoring display for a ping pong table using a WS2815 LED strip.

## Hardware

- **Board**: QuinLED-Dig-Uno v3.5 (pre-assembled) with ESP32-WROOM-32E
- **LEDs**: WS2815 (12V) addressable strip, 144 LEDs/m
- **Buttons**: 2x momentary push buttons (normally open)
- **Power**: 12V power supply (sized for your LED count)

## Wiring

### LED Strip (QuinLED-Dig-Uno 4-pin terminal block)
| Strip Wire | Terminal | GPIO |
|-----------|----------|------|
| Data (green) | LED1 | 16 |
| Backup data (blue) | LED2 | — |
| +12V (red) | V+ | — |
| GND (white) | GND | — |

### Buttons (QuinLED-Dig-Uno Q pads, active LOW with internal pull-up)
| Button | Pad | GPIO |
|--------|-----|------|
| Player 1 | Q1 | 15 |
| Player 2 | Q2 | 12 |

Wire each button between the Q pad and GND. No external resistors needed — the code enables internal pull-ups.

### Important Notes
- **Common ground**: The ESP32 GND must be connected to the LED power supply GND.
- **Level shifting**: The pre-assembled QuinLED-Dig-Uno includes level shifting on the LED1/LED2 outputs.
- **FastLED driver**: Use RMT, not I2S (`-D FASTLED_RMT_MAX_CHANNELS=2`). The I2S driver causes incorrect LED behavior on this board.
- **Serial disabled**: UART TX/RX pins (GPIO1/GPIO3) may conflict with LED outputs. Debug logging is available via telnet instead (see below).

## Setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension recommended)
2. Open this folder as a PlatformIO project
3. Edit `include/config.h`:
   - Set `TOTAL_LEDS` to match your strip length
   - Verify GPIO pin assignments for your board
   - Adjust `BRIGHTNESS` as needed
4. Copy `include/secrets.h.example` to `include/secrets.h` and fill in your WiFi credentials
5. First flash via USB: `pio run -e esp32-usb -t upload`
6. Subsequent flashes via OTA: `pio run -e esp32-ota -t upload`
7. Monitor logs via telnet: `nc pingpong-scorer.local 23`

## How It Works

### LED Layout
```
Strip across center of table:

  P1 side                                              P2 side
  [score →→→→→→→→→→→→→→→→→→→→→][serve|gap|serve][←←←←←←←←←←←←←←←←←←←←← score]
  LED 0                    LED 20  21 ... 66  67                         LED 87
```

- Each player has 21 LEDs growing inward from their edge
- Score LEDs light up from the outside in as points are scored
- A pulsing white LED just past the score area indicates who serves
- The center gap LEDs are used for serve-change animations

### Score Colors (every 5 points)
| Points | Color |
|--------|-------|
| 1-5 | 🟢 Green |
| 6-10 | 🔵 Blue |
| 11-15 | 🟣 Purple |
| 16-20 | 🟠 Orange-Red |
| 21 | 🔴 Red |

### Controls
| Action | Input |
|--------|-------|
| Player 1 scores | Press P1 button |
| Player 2 scores | Press P2 button |
| Reset game | Hold BOTH buttons for 3 seconds |
| New game after win | Press any button (after victory animation) |

### Game Rules (21-point classic)
- First to 21 wins
- Serve switches every 5 points
- At 20-20 (deuce): serve switches every 2 points, must win by 2
- First server alternates between games

## Customization

### Change pin assignments
Edit the `PIN CONFIGURATION` section in `config.h`.

### Change colors
Edit `SCORE_COLORS[]` in `config.h`. Uses FastLED CRGB color names.

### Change game rules
Edit `POINTS_TO_WIN`, `SERVE_SWITCH_EVERY`, etc. in `config.h`.

### LED density
If your strip has fewer LEDs, reduce `TOTAL_LEDS` and optionally `SCORE_LEDS_PER_SIDE` if you want fewer than 21 LEDs per player (in which case multiple points may share a single LED — you'd need to modify the display code).

## OTA & Telnet Logging

After the first USB flash, the board connects to WiFi and supports:
- **OTA updates**: Flash wirelessly with `pio run -e esp32-ota -t upload`
- **Telnet logging** (port 23): All debug output is mirrored over the network since Serial is disabled. Connect with `nc pingpong-scorer.local 23` to see live logs.

## Troubleshooting

- **No LEDs light up**: Check data pin, power connections, and that GND is shared between ESP32 and LED power supply.
- **LEDs all white / wrong colors**: Make sure you are using the RMT driver, not I2S. Check `build_flags` in `platformio.ini` for `-D FASTLED_RMT_MAX_CHANNELS=2`. Also try changing `COLOR_ORDER` in config.h (GRB, RGB, BRG).
- **Buttons not working**: Check wiring. Connect via telnet to see button press logs. On the QuinLED-Dig-Uno, use the Q1-Q4 pads for button inputs (not the MOSFET-driven LED/Q outputs on the terminal blocks).
- **LEDs too bright/dim**: Adjust `BRIGHTNESS` in config.h (0-255).
- **OTA flash fails**: Make sure the board is powered on and connected to WiFi. If it crashed, power cycle it first.
