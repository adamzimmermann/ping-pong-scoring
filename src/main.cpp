// =============================================================================
// PING PONG SCORING SYSTEM
// =============================================================================
// ESP32-WROOM-32E (QuinLED) + WS2815 LED Strip
//
// Controls:
//   - Player 1 button: adds a point for Player 1 (left side)
//   - Player 2 button: adds a point for Player 2 (right side)
//   - Hold BOTH buttons for 3 seconds: reset the game
//
// LED Layout (single strip across table center):
//   [P1 score: grows right ->] [gap w/ serve indicator] [<- grows left: P2 score]
//
// Score colors change every 5 points for easy reading at a distance.
// Serve indicator pulses on the serving player's side.
// Animations play on serve change and game win.
// =============================================================================

#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "game.h"
#include "display.h"
#include "netlog.h"

// =============================================================================
// GLOBALS
// =============================================================================

PingPongGame game;
ScoreDisplay display;
DualPrint logger;

// Button state tracking
struct ButtonState {
    uint8_t pin;
    bool lastState;
    bool currentState;
    unsigned long lastPressTime;
    bool pressed;  // Edge-detected: true for one loop when freshly pressed

    void begin(uint8_t _pin) {
        pin = _pin;
        pinMode(pin, INPUT_PULLUP);  // Buttons connect pin to GND
        lastState = HIGH;
        currentState = HIGH;
        lastPressTime = 0;
        pressed = false;
    }

    void update() {
        pressed = false;
        currentState = digitalRead(pin);

        // Detect falling edge (HIGH -> LOW) with debounce
        if (currentState == LOW && lastState == HIGH) {
            if (millis() - lastPressTime > DEBOUNCE_MS) {
                pressed = true;
                lastPressTime = millis();
            }
        }
        lastState = currentState;
    }

    bool isHeld() const {
        return (currentState == LOW);
    }

    unsigned long heldDuration() const {
        if (currentState == LOW) {
            return millis() - lastPressTime;
        }
        return 0;
    }
};

ButtonState btn1, btn2;
unsigned long bothHeldSince = 0;
bool resetTriggered = false;

// =============================================================================
// DEBUG OUTPUT
// =============================================================================

void printGameState() {
    logger.print("Score: P1=");
    logger.print(game.score[0]);
    logger.print(" P2=");
    logger.print(game.score[1]);
    logger.print(" | Serve: P");
    logger.print(game.servingPlayer + 1);
    if (game.isDeuce()) {
        logger.print(" [DEUCE]");
    }
    if (game.isGameWon()) {
        logger.print(" >>> WINNER: P");
        logger.print(game.winner() + 1);
        logger.print(" <<<");
    }
    logger.println();
}

// =============================================================================
// STATE HANDLERS
// =============================================================================

void handlePlaying() {
    // Process button presses (only when not doing reset)
    if (!resetTriggered) {
        if (btn1.pressed) {
            logger.print("Player 1 scores! ");
            game.addPoint(0);
            printGameState();
        }
        if (btn2.pressed) {
            logger.print("Player 2 scores! ");
            game.addPoint(1);
            printGameState();
        }
    }

    // Render the current score
    if (game.totalPoints() == 0) {
        display.renderIdle(game);
    } else {
        display.renderPlaying(game);
    }
}

void handleServeChange() {
    bool done = display.animateServeChange(game);
    if (done) {
        game.state = GameState::PLAYING;
        logger.print("Serve now: Player ");
        logger.println(game.servingPlayer + 1);
    }
}

void handleGameOver() {
    bool done = display.animateVictory(game);

    if (done) {
        // After victory animation, show final score with loser dimmed
        display.renderGameOver(game);

        // Flash winner side periodically
        static unsigned long lastFlash = 0;
        if (millis() - lastFlash > 1000) {
            lastFlash = millis();
        }
    }

    // Any button press after game over starts a new game
    if (btn1.pressed || btn2.pressed) {
        unsigned long elapsed = millis() - game.animStartTime;
        if (elapsed > 3000) {  // Wait at least 3 seconds before allowing reset
            logger.println(">>> NEW GAME <<<");
            int8_t loser = 1 - game.winner();  // Loser serves first next game
            game.reset();
            game.firstServer = loser;
            game.servingPlayer = game.firstServer;
            display.clearAll();
            display.show();
            delay(300);
            printGameState();
        }
    }
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    // Serial disabled — UART TX/RX pins may conflict with LED outputs
    // Use telnet (nc pingpong-scorer.local 23) for logging instead
    logger.println();
    logger.println("=== Ping Pong Scorer ===");
    logger.println("Initializing...");

    // Initialize WiFi for OTA updates
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    logger.print("Connecting to WiFi");

    // Wait up to 10 seconds for WiFi — don't block forever
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
        delay(250);
        logger.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        logger.println();
        logger.print("WiFi connected! IP: ");
        logger.println(WiFi.localIP());

        // Setup OTA
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        #ifdef OTA_PASSWORD
        ArduinoOTA.setPassword(OTA_PASSWORD);
        #endif

        ArduinoOTA.onStart([]() {
            // Blank LEDs during OTA to reduce power draw / interference
            FastLED.clear();
            FastLED.show();
            logger.println("OTA update starting...");
        });
        ArduinoOTA.onEnd([]() {
            logger.println("\nOTA update complete! Rebooting...");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            logger.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            logger.printf("OTA Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) logger.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) logger.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) logger.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) logger.println("Receive Failed");
            else if (error == OTA_END_ERROR) logger.println("End Failed");
        });
        ArduinoOTA.begin();
        logger.begin();
        logger.println("OTA ready. Telnet logging on port " + String(TELNET_PORT));
    } else {
        logger.println();
        logger.println("WiFi failed — running without OTA. Game still works!");
    }

    // Initialize buttons
    btn1.begin(BUTTON_PLAYER1_PIN);
    btn2.begin(BUTTON_PLAYER2_PIN);

    // Initialize display
    display.begin();
    display.animateStartup();

    // Initialize game
    game.reset();
    printGameState();

    logger.println("Ready! Press buttons to score.");
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
    // Handle OTA updates and telnet connections
    ArduinoOTA.handle();
    logger.handle();

    // Update button states
    btn1.update();
    btn2.update();

    // Check for simultaneous long-press reset
    if (btn1.isHeld() && btn2.isHeld()) {
        if (bothHeldSince == 0) {
            bothHeldSince = millis();
        } else if (millis() - bothHeldSince >= LONG_PRESS_MS && !resetTriggered) {
            resetTriggered = true;
            logger.println(">>> GAME RESET <<<");
            game.reset();
            display.clearAll();
            display.show();
            delay(200);
            display.animateStartup();
            printGameState();
        }
    } else {
        bothHeldSince = 0;
        resetTriggered = false;
    }

    // State machine
    switch (game.state) {
        case GameState::PLAYING:
            handlePlaying();
            break;

        case GameState::SERVE_CHANGE:
            handleServeChange();
            break;

        case GameState::GAME_OVER:
            handleGameOver();
            break;
    }

    // Small delay to avoid hammering the LEDs
    delay(16);  // ~60fps
}
