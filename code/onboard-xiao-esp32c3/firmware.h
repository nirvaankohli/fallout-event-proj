#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "../common/esp_now_message.h"

namespace onboard_xiao {

using fallout_event::BlinkCommand;
using fallout_event::kEspNowChannel;
using fallout_event::kProtocolMagic;

constexpr uint32_t kUsbBaud = 115200;
constexpr int kLedPin = D10;

portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;
BlinkCommand pendingCommand = {};
uint8_t pendingSourceMac[6] = {};
volatile bool hasPendingCommand = false;
bool ledIsOn = false;
unsigned long ledOffAtMs = 0;
uint32_t receivedMessages = 0;

void setLed(bool enabled) {
  ledIsOn = enabled;
  digitalWrite(kLedPin, enabled ? HIGH : LOW);
}

void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len) {
  if (len != static_cast<int>(sizeof(BlinkCommand))) {
    return;
  }

  const BlinkCommand *incoming = reinterpret_cast<const BlinkCommand *>(data);
  if (incoming->magic != kProtocolMagic) {
    return;
  }

  portENTER_CRITICAL(&commandMux);
  memcpy(&pendingCommand, incoming, sizeof(BlinkCommand));
  memcpy(pendingSourceMac, macAddr, sizeof(pendingSourceMac));
  hasPendingCommand = true;
  portEXIT_CRITICAL(&commandMux);
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  delay(100);

  if (esp_wifi_set_channel(kEspNowChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("Failed to set WiFi channel");
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  if (esp_now_register_recv_cb(onDataRecv) != ESP_OK) {
    Serial.println("Failed to register receive callback");
    return false;
  }

  return true;
}

void handlePendingCommand() {
  if (!hasPendingCommand) {
    return;
  }

  BlinkCommand command = {};
  uint8_t sourceMac[6] = {};

  portENTER_CRITICAL(&commandMux);
  memcpy(&command, &pendingCommand, sizeof(BlinkCommand));
  memcpy(sourceMac, pendingSourceMac, sizeof(sourceMac));
  hasPendingCommand = false;
  portEXIT_CRITICAL(&commandMux);

  receivedMessages++;
  setLed(true);
  ledOffAtMs = millis() + command.durationMs;

  Serial.printf("[RECV #%lu] from %02X:%02X:%02X:%02X:%02X:%02X text=\"%s\" blink=%lums\n",
                static_cast<unsigned long>(command.sequence),
                sourceMac[0], sourceMac[1], sourceMac[2],
                sourceMac[3], sourceMac[4], sourceMac[5],
                command.text,
                static_cast<unsigned long>(command.durationMs));
}

void serviceLedTimer() {
  if (ledIsOn && millis() >= ledOffAtMs) {
    setLed(false);
    Serial.printf("[LED] OFF after message count=%lu\n", static_cast<unsigned long>(receivedMessages));
  }
}

void setup() {
  pinMode(kLedPin, OUTPUT);
  setLed(false);

  Serial.begin(kUsbBaud);
  delay(250);

  if (!initEspNow()) {
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  Serial.println();
  Serial.println("Onboard XIAO ESP32-C3 ready");
  Serial.println("Waiting for ESP-NOW blink commands from the laptop-connected XIAO");
  Serial.println("LED output pin: D10");
  Serial.println("WiFi mode: STA");
  Serial.println("Channel: 6");
  Serial.println("MAC: " + WiFi.macAddress());
  Serial.println();
}

void loop() {
  handlePendingCommand();
  serviceLedTimer();
  delay(2);
}

}  // namespace onboard_xiao
