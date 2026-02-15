#pragma once

#include <FastLED.h>

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
// Adjust these to match your QuinLED board's available GPIOs.
// QuinLED-Dig-Uno typically uses GPIO16 for LED data.
// Check your specific board's pinout documentation.

#define LED_DATA_PIN        16    // LED1 output on QuinLED Dig-Uno (GPIO16)
#define BUTTON_PLAYER1_PIN  15    // Player 1 (left side) button — Q1 on QuinLED Dig-Uno v3
#define BUTTON_PLAYER2_PIN  12    // Player 2 (right side) button — Q2 on QuinLED Dig-Uno v3

// =============================================================================
// LED STRIP CONFIGURATION
// =============================================================================
// WS2815 is 12V but uses the same protocol as WS2812B from FastLED's perspective.
// 144 LEDs/m strip - adjust TOTAL_LEDS to your actual strip length.

#define LED_TYPE            WS2812B   // WS2815 uses same protocol
#define COLOR_ORDER         GRB
#define TOTAL_LEDS          144       // 144 LEDs/m, 1m strip
#define BRIGHTNESS          80        // 0-255, start conservative

// How many LEDs per player side for scoring (21 points with gaps = 41 LEDs)
#define SCORE_LEDS_PER_SIDE 41

// LED layout: Player 1 LEDs are at the LEFT edge, Player 2 at the RIGHT edge.
// Score LEDs grow INWARD from each edge toward center.
//
// [P1 score: 0..40] [serve:41] [gap:42..101] [serve:102] [P2 score: 103..143]
//  ^ Every other LED lit (0,2,4..40)        ^ P2 mirrored (143,141,139..103)
//
// P1 lights up LEDs 0,1,2... as they score
// P2 lights up LEDs (TOTAL_LEDS-1), (TOTAL_LEDS-2)... as they score

// Serve indicator LED: the outermost LED on the serving player's side pulses
// P1 serve indicator = LED index (SCORE_LEDS_PER_SIDE) (just past score area)
// P2 serve indicator = LED index (TOTAL_LEDS - SCORE_LEDS_PER_SIDE - 1)

// =============================================================================
// GAME RULES - 21 point classic
// =============================================================================
#define POINTS_TO_WIN       21
#define SERVE_SWITCH_EVERY  5     // Switch serve every 5 points normally
#define DEUCE_SERVE_SWITCH  2     // Switch serve every 2 points at deuce (20-20+)
#define DEUCE_THRESHOLD     20    // When both players reach this, deuce rules apply
#define WIN_BY              2     // Must win by 2 after deuce

// =============================================================================
// BUTTON DEBOUNCE
// =============================================================================
#define DEBOUNCE_MS         100   // Minimum ms between button presses
#define DOUBLE_TAP_MS       400   // Window for double-tap detection
#define LONG_PRESS_MS       3000  // Hold either button this long to reset

// =============================================================================
// SCORE COLORS - each group of 5 points gets a distinct color
// =============================================================================
// Colors for points 1-5, 6-10, 11-15, 16-20, 21 (game point)
const CRGB SCORE_COLORS[] = {
    CRGB::Blue,            // Points  1-5
    CRGB::Green,           // Points  6-10
    CRGB::Yellow,          // Points 11-15
    CRGB::OrangeRed,       // Points 16-20
    CRGB::Red              // Point  21 (game point!)
};
#define NUM_SCORE_COLORS (sizeof(SCORE_COLORS) / sizeof(SCORE_COLORS[0]))

// Serve indicator color
#define SERVE_COLOR         CRGB::White

// Serve change animation color
#define SERVE_ANIM_COLOR    CRGB::White

// Deuce: advantage indicator LED color
#define DEUCE_ADV_COLOR     CRGB::Red

// Game over: losing player's LEDs are dimmed to this level (0-255)
#define LOSER_DIM           20

// Victory winner flash color
#define VICTORY_FLASH_COLOR CRGB::White

// Background (unlit score LEDs)
#define BG_COLOR            CRGB::Black

// Animation timing
#define SERVE_PULSE_SPEED   3     // Speed of serve indicator pulse (lower = faster)
#define ANIMATION_SPEED_MS  50    // Frame delay for animations

// =============================================================================
// WIFI & OTA CONFIGURATION
// =============================================================================
// Credentials are loaded from secrets.h (not tracked in git).
// Copy secrets.h.example to secrets.h and fill in your values.

// =============================================================================
// TELNET SERIAL MONITOR
// =============================================================================
#define TELNET_PORT         23

#include "secrets.h"
