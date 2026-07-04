#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

namespace laptop_xiao {

constexpr uint32_t kUsbBaud = 115200;
constexpr char kApSsid[] = "fallout-xiao-link";
constexpr char kApPassword[] = "fallout123";
constexpr uint8_t kWifiChannel = 6;
constexpr uint16_t kCommandPort = 4210;
constexpr uint16_t kAckPort = 4211;
const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kSubnet(255, 255, 255, 0);
const IPAddress kOnboardIp(192, 168, 4, 2);

WiFiUDP commandUdp;
WiFiUDP ackUdp;
String usbLineBuffer;
String ackLineBuffer;
uint32_t sentSequence = 0;

void printBanner() {
  Serial.println();
  Serial.println("Laptop-connected XIAO ESP32-C3 ready");
  Serial.println("USB serial -> WiFi UDP -> onboard XIAO");
  Serial.println("The onboard XIAO sends an ACK line back for each message.");
  Serial.println("AP SSID: fallout-xiao-link");
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  Serial.println("Onboard target IP: " + kOnboardIp.toString());
  Serial.println("Command port: 4210");
  Serial.println("ACK port: 4211");
  Serial.println();
}

bool initWiFiLink() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAPConfig(kApIp, kApIp, kSubnet)) {
    Serial.println("Failed to configure AP IP");
    return false;
  }

  if (!WiFi.softAP(kApSsid, kApPassword, kWifiChannel, false, 1)) {
    Serial.println("Failed to start AP");
    return false;
  }

  if (!ackUdp.begin(kAckPort)) {
    Serial.println("Failed to open ACK UDP port");
    return false;
  }

  return true;
}

void sendLine(const String &line) {
  if (line.isEmpty()) {
    return;
  }

  sentSequence++;
  commandUdp.beginPacket(kOnboardIp, kCommandPort);
  commandUdp.print(line);
  commandUdp.endPacket();
  Serial.printf("[SEND #%lu] \"%s\"\n", static_cast<unsigned long>(sentSequence), line.c_str());
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

void pumpAckInput() {
  const int packetSize = ackUdp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  while (ackUdp.available()) {
    const char incoming = static_cast<char>(ackUdp.read());
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      if (!ackLineBuffer.isEmpty()) {
        Serial.println("[ACK] " + ackLineBuffer);
        ackLineBuffer = "";
      }
      continue;
    }
    ackLineBuffer += incoming;
  }

  if (!ackLineBuffer.isEmpty()) {
    Serial.println("[ACK] " + ackLineBuffer);
    ackLineBuffer = "";
  }
}

void setup() {
  Serial.begin(kUsbBaud);
  delay(250);

  if (!initWiFiLink()) {
    Serial.println("WiFi link init failed; rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  printBanner();
}

void loop() {
  pumpUsbInput();
  pumpAckInput();
  delay(2);
}

}  // namespace laptop_xiao
