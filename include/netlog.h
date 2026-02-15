#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// =============================================================================
// DualPrint — mirrors Serial output to a telnet client over WiFi
// =============================================================================
// Usage:
//   DualPrint logger;
//   logger.begin();          // after WiFi connects
//   logger.handle();         // in loop()
//   logger.println("hello"); // prints to Serial AND telnet

class DualPrint : public Print {
public:
    void begin() {
        _server = new WiFiServer(TELNET_PORT);
        _server->begin();
        _server->setNoDelay(true);
    }

    void handle() {
        if (!_server) return;

        // Accept new connections (only one client at a time)
        if (_server->hasClient()) {
            if (_client && _client.connected()) {
                _client.stop();
            }
            _client = _server->available();
            _client.println("=== Ping Pong Scorer telnet log ===");
        }

        // Drop disconnected client
        if (_client && !_client.connected()) {
            _client.stop();
        }
    }

    size_t write(uint8_t b) override {
        if (_serialEnabled) Serial.write(b);
        if (_client && _client.connected()) {
            _client.write(b);
        }
        return 1;
    }

    size_t write(const uint8_t *buf, size_t size) override {
        if (_serialEnabled) Serial.write(buf, size);
        if (_client && _client.connected()) {
            _client.write(buf, size);
        }
        return size;
    }

    void enableSerial(bool enabled) { _serialEnabled = enabled; }

private:
    WiFiServer *_server = nullptr;
    WiFiClient _client;
    bool _serialEnabled = false;
};
