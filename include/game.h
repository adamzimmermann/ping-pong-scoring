#pragma once

#include <Arduino.h>
#include "config.h"

// =============================================================================
// GAME STATE
// =============================================================================

enum class GameState {
    PLAYING,          // Normal gameplay
    SERVE_CHANGE,     // Serve change animation playing
    GAME_OVER         // Someone won, victory animation
};

struct PingPongGame {
    uint8_t score[2];       // score[0] = Player 1, score[1] = Player 2
    uint8_t servingPlayer;  // 0 or 1
    uint8_t firstServer;    // Who served first this game (for serve tracking)
    GameState state;
    unsigned long animStartTime;

    void reset() {
        score[0] = 0;
        score[1] = 0;
        // Alternate who serves first each game, or default to P1
        firstServer = 0;
        servingPlayer = firstServer;
        state = GameState::PLAYING;
        animStartTime = 0;
    }

    // Total points played in the game
    uint16_t totalPoints() const {
        return (uint16_t)score[0] + (uint16_t)score[1];
    }

    // Are we in deuce territory?
    bool isDeuce() const {
        return score[0] >= DEUCE_THRESHOLD && score[1] >= DEUCE_THRESHOLD;
    }

    // Current serve switch interval
    uint8_t currentServeInterval() const {
        return isDeuce() ? DEUCE_SERVE_SWITCH : SERVE_SWITCH_EVERY;
    }

    // Calculate who should be serving based on total points
    uint8_t calculateServingPlayer() const {
        uint16_t total = totalPoints();

        if (!isDeuce()) {
            // Normal play: switch every SERVE_SWITCH_EVERY points
            uint8_t serveBlock = total / SERVE_SWITCH_EVERY;
            return (firstServer + serveBlock) % 2;
        } else {
            // Deuce: first figure out how many points were played before deuce
            uint16_t pointsBeforeDeuce = DEUCE_THRESHOLD * 2;
            uint16_t blocksBeforeDeuce = pointsBeforeDeuce / SERVE_SWITCH_EVERY;

            // Points played during deuce
            uint16_t deucePoints = total - pointsBeforeDeuce;
            uint16_t deuceBlocks = deucePoints / DEUCE_SERVE_SWITCH;

            return (firstServer + blocksBeforeDeuce + deuceBlocks) % 2;
        }
    }

    // Add a point. Returns true if serve changes.
    bool addPoint(uint8_t player) {
        if (state != GameState::PLAYING) return false;
        if (player > 1) return false;

        score[player]++;
        uint8_t newServer = calculateServingPlayer();
        bool serveChanged = (newServer != servingPlayer);
        servingPlayer = newServer;

        // Check for win
        if (isGameWon()) {
            state = GameState::GAME_OVER;
            animStartTime = millis();
            return false;
        }

        if (serveChanged) {
            state = GameState::SERVE_CHANGE;
            animStartTime = millis();
        }

        return serveChanged;
    }

    // Remove a point from a player (for undo via double-tap)
    void removePoint(uint8_t player) {
        if (player > 1 || score[player] == 0) return;
        score[player]--;
        servingPlayer = calculateServingPlayer();
        // If game was over, go back to playing
        if (state == GameState::GAME_OVER) {
            state = GameState::PLAYING;
        }
    }

    // Check if someone has won
    bool isGameWon() const {
        for (int p = 0; p < 2; p++) {
            if (score[p] >= POINTS_TO_WIN) {
                // If both reached deuce threshold, must win by WIN_BY
                if (score[0] >= DEUCE_THRESHOLD && score[1] >= DEUCE_THRESHOLD) {
                    if ((int)score[p] - (int)score[1 - p] >= WIN_BY) return true;
                } else {
                    return true;
                }
            }
        }
        return false;
    }

    // Return the winner (0 or 1), or -1 if no winner
    int8_t winner() const {
        if (!isGameWon()) return -1;
        return (score[0] > score[1]) ? 0 : 1;
    }
};
