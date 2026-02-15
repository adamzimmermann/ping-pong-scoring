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

// =============================================================================
// GLOBALS
// =============================================================================

PingPongGame game;
ScoreDisplay display;

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
    Serial.print("Score: P1=");
    Serial.print(game.score[0]);
    Serial.print(" P2=");
    Serial.print(game.score[1]);
    Serial.print(" | Serve: P");
    Serial.print(game.servingPlayer + 1);
    if (game.isDeuce()) {
        Serial.print(" [DEUCE]");
    }
    if (game.isGameWon()) {
        Serial.print(" >>> WINNER: P");
        Serial.print(game.winner() + 1);
        Serial.print(" <<<");
    }
    Serial.println();
}

// =============================================================================
// STATE HANDLERS
// =============================================================================

void handlePlaying() {
    // Process button presses (only when not doing reset)
    if (!resetTriggered) {
        if (btn1.pressed) {
            Serial.print("Player 1 scores! ");
            game.addPoint(0);
            printGameState();
        }
        if (btn2.pressed) {
            Serial.print("Player 2 scores! ");
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
        Serial.print("Serve now: Player ");
        Serial.println(game.servingPlayer + 1);
    }
}

void handleGameOver() {
    bool done = display.animateVictory(game);

    if (done) {
        // After victory animation, show final score and wait for reset
        display.renderPlaying(game);

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
            Serial.println(">>> NEW GAME <<<");
            uint8_t lastFirstServer = game.firstServer;
            game.reset();
            game.firstServer = 1 - lastFirstServer;  // Alternate first serve
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
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Ping Pong Scorer ===");
    Serial.println("Initializing...");

    // Initialize WiFi for OTA updates
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    // Wait up to 10 seconds for WiFi — don't block forever
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
        delay(250);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi connected! IP: ");
        Serial.println(WiFi.localIP());

        // Setup OTA
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        #ifdef OTA_PASSWORD
        ArduinoOTA.setPassword(OTA_PASSWORD);
        #endif

        ArduinoOTA.onStart([]() {
            // Blank LEDs during OTA to reduce power draw / interference
            FastLED.clear();
            FastLED.show();
            Serial.println("OTA update starting...");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA update complete! Rebooting...");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("OTA Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });
        ArduinoOTA.begin();
        Serial.println("OTA ready.");
    } else {
        Serial.println();
        Serial.println("WiFi failed — running without OTA. Game still works!");
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

    Serial.println("Ready! Press buttons to score.");
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
    // Handle OTA updates (must be called frequently)
    ArduinoOTA.handle();

    // Update button states
    btn1.update();
    btn2.update();

    // Check for simultaneous long-press reset
    if (btn1.isHeld() && btn2.isHeld()) {
        if (bothHeldSince == 0) {
            bothHeldSince = millis();
        } else if (millis() - bothHeldSince >= LONG_PRESS_MS && !resetTriggered) {
            resetTriggered = true;
            Serial.println(">>> GAME RESET <<<");
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