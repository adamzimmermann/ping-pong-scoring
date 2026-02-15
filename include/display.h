#pragma once

#include <FastLED.h>
#include "config.h"
#include "game.h"

// =============================================================================
// LED DISPLAY
// =============================================================================

class ScoreDisplay {
public:
    CRGB leds[TOTAL_LEDS];

    void begin() {
        FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, TOTAL_LEDS)
            .setCorrection(TypicalLEDStrip);
        FastLED.setBrightness(BRIGHTNESS);
        clearAll();
        FastLED.show();
    }

    void clearAll() {
        fill_solid(leds, TOTAL_LEDS, BG_COLOR);
    }

    // =========================================================================
    // SCORE RENDERING
    // =========================================================================

    // Get the color for a given point number (1-based)
    CRGB colorForPoint(uint8_t pointNum) {
        if (pointNum == 0) return BG_COLOR;
        uint8_t groupIndex = (pointNum - 1) / 5;
        if (groupIndex >= NUM_SCORE_COLORS) groupIndex = NUM_SCORE_COLORS - 1;
        return SCORE_COLORS[groupIndex];
    }

    // Render a player's score on the strip
    // Player 0 (left):  LEDs grow from index 0 inward
    // Player 1 (right): LEDs grow from index (TOTAL_LEDS-1) inward
    void renderScore(const PingPongGame& game) {
        // Clear score areas first
        for (int i = 0; i < SCORE_LEDS_PER_SIDE; i++) {
            leds[i] = BG_COLOR;
            leds[TOTAL_LEDS - 1 - i] = BG_COLOR;
        }

        // Player 1 score (left side, growing right from edge)
        for (int i = 0; i < game.score[0] && i < SCORE_LEDS_PER_SIDE; i++) {
            leds[i] = colorForPoint(i + 1);
        }

        // Player 2 score (right side, growing left from edge)
        for (int i = 0; i < game.score[1] && i < SCORE_LEDS_PER_SIDE; i++) {
            leds[TOTAL_LEDS - 1 - i] = colorForPoint(i + 1);
        }

        // Deuce advantage: dim the trailing player's LEDs
        if (game.isDeuce()) {
            for (int p = 0; p < 2; p++) {
                if (game.score[p] < game.score[1 - p]) {
                    // This player is trailing — dim their LEDs
                    for (int i = 0; i < SCORE_LEDS_PER_SIDE; i++) {
                        int idx = (p == 0) ? i : (TOTAL_LEDS - 1 - i);
                        leds[idx].nscale8(DEUCE_TRAILING_DIM);
                    }
                }
            }
        }
    }

    // =========================================================================
    // SERVE INDICATOR
    // =========================================================================

    // Pulse a serve indicator LED just outside the score area
    void renderServeIndicator(const PingPongGame& game) {
        // Serve indicator positions: just past the score area on each side
        int p1ServeIdx = SCORE_LEDS_PER_SIDE;                  // First LED past P1's score
        int p2ServeIdx = TOTAL_LEDS - SCORE_LEDS_PER_SIDE - 1; // First LED past P2's score

        // Clear both serve indicators
        if (p1ServeIdx < TOTAL_LEDS) leds[p1ServeIdx] = BG_COLOR;
        if (p2ServeIdx >= 0)         leds[p2ServeIdx] = BG_COLOR;

        // Pulse the active server's indicator
        uint8_t pulse = beatsin8(60 / SERVE_PULSE_SPEED, 40, 255);
        CRGB serveCol = SERVE_COLOR;
        serveCol.nscale8(pulse);

        if (game.servingPlayer == 0 && p1ServeIdx < TOTAL_LEDS) {
            leds[p1ServeIdx] = serveCol;
        } else if (game.servingPlayer == 1 && p2ServeIdx >= 0) {
            leds[p2ServeIdx] = serveCol;
        }
    }

    // =========================================================================
    // ANIMATIONS
    // =========================================================================

    // Serve change animation: chase pattern across the center gap
    // Returns true when animation is complete
    bool animateServeChange(const PingPongGame& game) {
        unsigned long elapsed = millis() - game.animStartTime;
        int totalFrames = 30;  // ~1.5 seconds at 50ms/frame
        int frame = elapsed / ANIMATION_SPEED_MS;

        if (frame >= totalFrames) return true;  // Animation done

        // Render the current score as base
        renderScore(game);

        // Chase animation: light up center gap LEDs in a sweep
        int gapStart = SCORE_LEDS_PER_SIDE;
        int gapEnd = TOTAL_LEDS - SCORE_LEDS_PER_SIDE - 1;
        int gapSize = gapEnd - gapStart + 1;

        if (gapSize > 0) {
            // Sweep toward the new server's indicator LED
            int sweepPos;
            if (game.servingPlayer == 1) {
                // New server is P2 (right): sweep left-to-right
                sweepPos = map(frame, 0, totalFrames, 0, gapSize - 1);
            } else {
                // New server is P1 (left): sweep right-to-left
                sweepPos = map(frame, 0, totalFrames, gapSize - 1, 0);
            }

            // Trail of 3 LEDs (trail behind the sweep direction)
            int trailDir = (game.servingPlayer == 1) ? -1 : 1;
            for (int t = 0; t < 3; t++) {
                int idx = gapStart + sweepPos + (t * trailDir);
                if (idx >= gapStart && idx <= gapEnd) {
                    uint8_t brightness = 255 - (t * 80);
                    leds[idx] = SERVE_ANIM_COLOR;
                    leds[idx].nscale8(brightness);
                }
            }
        }

        // Also flash the new serve indicator
        if ((frame / 3) % 2 == 0) {
            renderServeIndicator(game);
        }

        FastLED.show();
        return false;
    }

    // Victory animation: rainbow chase + winner's side flashing
    // Returns true when animation is complete (after ~5 seconds)
    bool animateVictory(const PingPongGame& game) {
        unsigned long elapsed = millis() - game.animStartTime;

        if (elapsed > 8000) return true;  // 8 seconds then stop

        int frame = elapsed / ANIMATION_SPEED_MS;

        // Rainbow sweep across entire strip
        for (int i = 0; i < TOTAL_LEDS; i++) {
            leds[i] = CHSV((i * 7) + (frame * 8), 255, 200);
        }

        // Flash the winner's score brighter
        int8_t w = game.winner();
        if (w >= 0 && (frame / 5) % 2 == 0) {
            for (int i = 0; i < game.score[w] && i < SCORE_LEDS_PER_SIDE; i++) {
                int idx = (w == 0) ? i : (TOTAL_LEDS - 1 - i);
                leds[idx] = VICTORY_FLASH_COLOR;
            }
        }

        FastLED.show();
        return false;
    }

    // Startup animation: quick rainbow test
    void animateStartup() {
        for (int i = 0; i < TOTAL_LEDS; i++) {
            leds[i] = CHSV(i * (256 / TOTAL_LEDS), 255, 180);
            FastLED.show();
            delay(10);
        }
        delay(500);

        // Fade out
        for (int b = 180; b >= 0; b -= 5) {
            FastLED.setBrightness(b);
            FastLED.show();
            delay(15);
        }
        FastLED.setBrightness(BRIGHTNESS);
        clearAll();
        FastLED.show();
    }

    // Normal frame: render score + serve indicator
    void renderPlaying(const PingPongGame& game) {
        clearAll();
        renderScore(game);
        renderServeIndicator(game);
        FastLED.show();
    }

    // Post-victory: show final score with loser's side dimmed
    void renderGameOver(const PingPongGame& game) {
        clearAll();
        renderScore(game);

        int8_t w = game.winner();
        if (w >= 0) {
            int loser = 1 - w;
            for (int i = 0; i < SCORE_LEDS_PER_SIDE; i++) {
                int idx = (loser == 0) ? i : (TOTAL_LEDS - 1 - i);
                leds[idx].nscale8(LOSER_DIM);
            }
        }

        FastLED.show();
    }

    // "Ready to play" idle: just show serve indicator pulsing
    void renderIdle(const PingPongGame& game) {
        clearAll();
        renderServeIndicator(game);
        FastLED.show();
    }

    void show() {
        FastLED.show();
    }
};
