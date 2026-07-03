#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "../common/esp_now_message.h"

namespace laptop_xiao {

using fallout_event::BlinkCommand;
using fallout_event::kEspNowChannel;

constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kBlinkDurationMs = 1000;
const uint8_t kBroadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

String usbLineBuffer;
uint32_t sentSequence = 0;

void printBanner() {
  Serial.println();
  Serial.println("Laptop-connected XIAO ESP32-C3 ready");
  Serial.println("Type a line and press Enter.");
  Serial.println("The message will be sent over ESP-NOW to the onboard XIAO.");
  Serial.println("Each received message makes the onboard LED turn on for 1 second.");
  Serial.println("Examples: hello, open, go, test");
  Serial.println("WiFi mode: STA");
  Serial.println("Channel: 6");
  Serial.println("MAC: " + WiFi.macAddress());
  Serial.println();
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

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcastAddress, sizeof(kBroadcastAddress));
  peer.channel = kEspNowChannel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW broadcast peer");
    return false;
  }

  return true;
}

void sendLine(const String &line) {
  if (line.length() == 0) {
    return;
  }

  const BlinkCommand command = fallout_event::makeBlinkCommand(++sentSequence, kBlinkDurationMs, line);
  const esp_err_t result = esp_now_send(kBroadcastAddress, reinterpret_cast<const uint8_t *>(&command), sizeof(command));

  if (result == ESP_OK) {
    Serial.printf("[SEND #%lu] \"%s\" -> blink 1000ms\n", static_cast<unsigned long>(command.sequence), command.text);
  } else {
    Serial.printf("[SEND FAILED] esp_now_send error=%d\n", static_cast<int>(result));
  }
}

void pumpUsbInput() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      usbLineBuffer.trim();
      sendLine(usbLineBuffer);
      usbLineBuffer = "";
      continue;
    }

    usbLineBuffer += incoming;
  }
}

void setup() {
  Serial.begin(kUsbBaud);
  delay(250);

  if (!initEspNow()) {
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  printBanner();
}

void loop() {
  pumpUsbInput();
  delay(2);
}

}  // namespace laptop_xiao
