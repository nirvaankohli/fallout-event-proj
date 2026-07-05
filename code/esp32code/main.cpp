#include <Arduino.h>
#include <WiFi.h>

namespace {

constexpr uint32_t kBaudRate = 115200;
constexpr uint16_t kWifiPort = 3333;
constexpr char kWifiSsid[] = "Nirvaan's Phone";
constexpr char kWifiPassword[] = "himynameisnirvaan";
constexpr char kFallbackAccessPointSsid[] = "FalloutESP32";
constexpr uint8_t kFallbackAccessPointChannel = 6;
IPAddress kFallbackAccessPointIp(192, 168, 4, 1);
IPAddress kFallbackAccessPointGateway(192, 168, 4, 1);
IPAddress kFallbackAccessPointSubnet(255, 255, 255, 0);

WiFiServer wifiServer(kWifiPort);
WiFiClient wifiClient;
bool wifiReady = false;

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

void startWifiAccessPoint() {
  WiFi.mode(WIFI_AP);
  const bool configOk =
      WiFi.softAPConfig(kFallbackAccessPointIp, kFallbackAccessPointGateway, kFallbackAccessPointSubnet);
  const bool started = WiFi.softAP(kFallbackAccessPointSsid, nullptr, kFallbackAccessPointChannel, 0, 1);
  if (!configOk) {
    Serial.println("[boot] Failed to configure Wi-Fi access point IP.");
  }
  if (!started) {
    Serial.println("[boot] Failed to start Wi-Fi access point.");
    return;
  }

  wifiServer.begin();
  wifiServer.setNoDelay(true);
  wifiReady = true;

  Serial.printf("[boot] Fallback Wi-Fi AP ready. SSID=%s security=open channel=%u\n",
                kFallbackAccessPointSsid,
                kFallbackAccessPointChannel);
  Serial.printf("[boot] Connect laptop to %s and open %s:%u\n",
                kFallbackAccessPointSsid,
                WiFi.softAPIP().toString().c_str(),
                kWifiPort);
}

void startWifiStation() {
  if (strlen(kWifiSsid) == 0) {
    Serial.println("[boot] No station Wi-Fi credentials configured.");
    startWifiAccessPoint();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(kWifiSsid, kWifiPassword);

  Serial.printf("[boot] Connecting to Wi-Fi SSID=%s\n", kWifiSsid);

  const uint32_t connectStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStart < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[boot] Station Wi-Fi connect failed. Starting fallback AP instead.");
    startWifiAccessPoint();
    return;
  }

  wifiServer.begin();
  wifiServer.setNoDelay(true);
  wifiReady = true;

  Serial.printf("[boot] Wi-Fi station connected. SSID=%s ip=%s port=%u\n",
                kWifiSsid,
                WiFi.localIP().toString().c_str(),
                kWifiPort);
}

void maintainWifiClient() {
  if (!wifiReady) {
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

  startWifiStation();
}

void loop() {
  const String serialMessage = readLineFrom(Serial);
  if (serialMessage.length() > 0) {
    handleMessage(serialMessage, Serial, "usb");
  }

  maintainWifiClient();
}
