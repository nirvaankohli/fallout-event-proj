#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

const int lfmotorpin1 = 2;
const int lfmotorpin2 = 3;
const int rfmotorpin1 = 4;
const int rfmotorpin2 = 5;
const int lbmotorpin1 = 20;
const int lbmotorpin2 = 8;
const int rbmotorpin1 = 9;
const int rbmotorpin2 = 10;

void setup() {
    pinMode(lfmotorpin1, OUTPUT);
    pinMode(lfmotorpin2, OUTPUT);
    pinMode(rfmotorpin1, OUTPUT);
    pinMode(rfmotorpin2, OUTPUT);
    pinMode(lbmotorpin1, OUTPUT);
    pinMode(lbmotorpin2, OUTPUT);
    pinMode(rbmotorpin1, OUTPUT);
    pinMode(rbmotorpin2, OUTPUT);
}

void loop() {
    runForward();
    delay(1000);
}

void runForward() {
    digitalWrite(lfmotorpin1, HIGH);
    digitalWrite(lfmotorpin2, LOW);
    digitalWrite(rfmotorpin1, LOW);
    digitalWrite(rfmotorpin2, HIGH);
    digitalWrite(lbmotorpin1, HIGH);
    digitalWrite(lbmotorpin2, LOW);
    digitalWrite(rbmotorpin1, LOW);
    digitalWrite(rbmotorpin2, HIGH);
}