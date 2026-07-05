#include <Arduino.h>
#include <WiFi.h>

namespace {

constexpr uint32_t kBaudRate = 115200;
constexpr uint16_t kWifiPort = 3333;
constexpr char kWifiSsid[] = "";
constexpr char kWifiPassword[] = "";

WiFiServer wifiServer(kWifiPort);
WiFiClient wifiClient;

String readLineFrom(Stream &stream) {
  static String buffer;

  while (stream.available() > 0) {
    const char nextChar = static_cast<char>(stream.read());

    if (nextChar == '\r') {
      continue;
    }

    if (nextChar == '\n') {
      const String line = buffer;
      buffer = "";
      return line;
    }

    buffer += nextChar;
  }

  return "";
}

void printHelp(Stream &stream) {
  stream.println("Commands:");
  stream.println("  ping");
  stream.println("  wifi status");
  stream.println("  help");
  stream.println("Any other text is echoed back.");
}

void handleMessage(const String &message, Stream &replyStream, const char *channelName) {
  if (message.length() == 0) {
    return;
  }

  if (message == "ping") {
    replyStream.printf("[%s] pong\n", channelName);
    return;
  }

  if (message == "wifi status") {
    if (WiFi.status() == WL_CONNECTED) {
      replyStream.printf(
          "[%s] wifi connected ssid=%s ip=%s port=%u\n",
          channelName,
          WiFi.SSID().c_str(),
          WiFi.localIP().toString().c_str(),
          kWifiPort);
    } else {
      replyStream.printf("[%s] wifi disabled or disconnected\n", channelName);
    }
    return;
  }

  if (message == "help") {
    printHelp(replyStream);
    return;
  }

  replyStream.printf("[%s] echo: %s\n", channelName, message.c_str());
}

void tryStartWifi() {
  if (strlen(kWifiSsid) == 0) {
    Serial.println("[boot] Wi-Fi disabled. Set kWifiSsid/kWifiPassword in main.cpp to enable TCP echo.");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);

  Serial.printf("[boot] Connecting to Wi-Fi SSID '%s'\n", kWifiSsid);

  const uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[boot] Wi-Fi connect timed out. USB serial echo is still available.");
    return;
  }

  wifiServer.begin();
  wifiServer.setNoDelay(true);
  Serial.printf("[boot] Wi-Fi ready at %s:%u\n", WiFi.localIP().toString().c_str(), kWifiPort);
}

void maintainWifiClient() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!wifiClient || !wifiClient.connected()) {
    WiFiClient newClient = wifiServer.available();
    if (newClient) {
      wifiClient.stop();
      wifiClient = newClient;
      wifiClient.setTimeout(10);
      wifiClient.println("[wifi] connected to ESP32 echo server");
      wifiClient.println("[wifi] send 'ping' or any line of text");
      Serial.printf("[wifi] client connected from %s\n", wifiClient.remoteIP().toString().c_str());
    }
    return;
  }

  while (wifiClient.available() > 0) {
    const String line = wifiClient.readStringUntil('\n');
    const String trimmed = line.substring(0, line.length());
    String message = trimmed;
    message.trim();
    handleMessage(message, wifiClient, "wifi");
    if (message.length() > 0) {
      Serial.printf("[wifi] rx: %s\n", message.c_str());
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(kBaudRate);

  const uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
    delay(10);
  }

  Serial.println();
  Serial.println("[boot] ESP32 echo bridge starting");
  Serial.printf("[boot] USB serial ready at %lu baud\n", static_cast<unsigned long>(kBaudRate));
  printHelp(Serial);

  tryStartWifi();
}

void loop() {
  const String serialMessage = readLineFrom(Serial);
  if (serialMessage.length() > 0) {
    handleMessage(serialMessage, Serial, "usb");
  }

  maintainWifiClient();
}
