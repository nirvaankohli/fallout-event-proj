#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

namespace onboard_xiao {

constexpr uint32_t kUsbBaud = 115200;
constexpr char kApSsid[] = "fallout-xiao-link";
constexpr char kApPassword[] = "fallout123";
constexpr uint16_t kCommandPort = 4210;
constexpr uint16_t kAckPort = 4211;
const IPAddress kOnboardIp(192, 168, 4, 2);
const IPAddress kGatewayIp(192, 168, 4, 1);
const IPAddress kSubnet(255, 255, 255, 0);

WiFiUDP commandUdp;
WiFiUDP ackUdp;
uint32_t receivedMessages = 0;

void printBanner() {
  Serial.println();
  Serial.println("Onboard XIAO ESP32-C3 ready");
  Serial.println("Connected to fallout-xiao-link");
  Serial.println("Waiting for UDP messages from the laptop-connected XIAO");
  Serial.println("This board sends an ACK line back for each message.");
  Serial.println("Local IP: " + WiFi.localIP().toString());
  Serial.println("Gateway IP: " + WiFi.gatewayIP().toString());
  Serial.println("Command port: 4210");
  Serial.println("ACK port: 4211");
  Serial.println();
}

bool connectToLaptopAp() {
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(kOnboardIp, kGatewayIp, kSubnet)) {
    Serial.println("Failed to configure static IP");
    return false;
  }

  WiFi.begin(kApSsid, kApPassword);
  const unsigned long startedAtMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startedAtMs > 15000) {
      Serial.println("Timed out connecting to laptop AP");
      return false;
    }
    delay(250);
  }

  if (!commandUdp.begin(kCommandPort)) {
    Serial.println("Failed to open command UDP port");
    return false;
  }

  return true;
}

void sendAck(const String &message) {
  ackUdp.beginPacket(kGatewayIp, kAckPort);
  ackUdp.print(message);
  ackUdp.endPacket();
}

void pumpCommandInput() {
  const int packetSize = commandUdp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  String line;
  while (commandUdp.available()) {
    const char incoming = static_cast<char>(commandUdp.read());
    if (incoming != '\r' && incoming != '\n') {
      line += incoming;
    }
  }

  line.trim();
  if (line.isEmpty()) {
    return;
  }

  receivedMessages++;
  Serial.printf("[RECV #%lu] \"%s\"\n", static_cast<unsigned long>(receivedMessages), line.c_str());
  sendAck("message #" + String(receivedMessages) + " received: " + line + "\n");
}

void setup() {
  Serial.begin(kUsbBaud);
  delay(250);

  if (!connectToLaptopAp()) {
    Serial.println("WiFi link init failed; rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  printBanner();
}

void loop() {
  pumpCommandInput();
  delay(2);
}

}  // namespace onboard_xiao
