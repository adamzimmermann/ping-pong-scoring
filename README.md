# Ping Pong Scoring System

ESP32-based scoring display for a ping pong table using a WS2815 LED strip.

## Hardware

- **Board**: QuinLED ESP32-WROOM-32E
- **LEDs**: WS2815 (12V) addressable strip, 144 LEDs/m
- **Buttons**: 2x momentary push buttons (normally open)
- **Power**: 12V power supply (sized for your LED count)

## Wiring

### LED Strip
| Strip Wire | Connect To |
|-----------|------------|
| Data (green) | GPIO 16 (or your QuinLED's LED data pin) |
| +12V (red) | 12V power supply + |
| GND (white) | Power supply GND + ESP32 GND |
| Backup data | Leave unconnected or tie to data |

### Buttons (active LOW with internal pull-up)
| Button | Connect To |
|--------|------------|
| Player 1 terminal A | GPIO 32 |
| Player 1 terminal B | GND |
| Player 2 terminal A | GPIO 33 |
| Player 2 terminal B | GND |

No external resistors needed — the code enables internal pull-ups.

### Important Notes
- **Common ground**: The ESP32 GND must be connected to the LED power supply GND.
- **GPIO pins**: The default pins (16, 32, 33) should work on most QuinLED boards. Check your specific board's documentation — some GPIOs may be used for onboard features. Avoid GPIO 0, 2, 5, 12, 15 (boot-sensitive pins).
- **Level shifting**: The QuinLED boards typically include level shifting on the LED data pin. If using a generic ESP32, you may need a level shifter (3.3V → 5V/12V logic).

## Setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension recommended)
2. Open this folder as a PlatformIO project
3. Edit `include/config.h`:
   - Set `TOTAL_LEDS` to match your strip length
   - Verify GPIO pin assignments for your board
   - Adjust `BRIGHTNESS` as needed
4. Build and upload: `pio run -t upload`
5. Open serial monitor at 115200 baud for debug output

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

## Troubleshooting

- **No LEDs light up**: Check data pin, power connections, and that GND is shared between ESP32 and LED power supply.
- **Wrong colors / flickering**: Try changing `COLOR_ORDER` in config.h (GRB, RGB, BRG). WS2815 strips vary.
- **Buttons not working**: Check wiring. Use serial monitor — it prints every button press and score change. Try swapping to different GPIO pins if a pin is reserved on your board.
- **LEDs too bright/dim**: Adjust `BRIGHTNESS` in config.h (0-255).
