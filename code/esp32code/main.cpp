#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

namespace {

constexpr uint32_t kBaudRate = 115200;
constexpr uint16_t kWebSocketPort = 3333;
constexpr uint32_t kCommandTimeoutMs = 300;
constexpr int kCommandDeadzone = 8;
constexpr int kBootMotorSpinCommand = 100;
constexpr uint32_t kBootMotorSpinMs = 500;
constexpr char kWifiSsid[] = "Nirvaan's Phone";
constexpr char kWifiPassword[] = "himynameisnirvaan";
constexpr char kFallbackAccessPointSsid[] = "FalloutESP32";
constexpr uint8_t kFallbackAccessPointChannel = 6;
constexpr uint8_t kActivityLedPin = 255;
constexpr uint8_t kSleepPin = 255;

struct MotorPins {
  uint8_t in1Pin;
  uint8_t in2Pin;
  bool invertDirection;
};

// These pin pairs match the temporary bench-test sketch that already spun all four motors.
constexpr MotorPins kLeftMotors[] = {
    {2, 3, false},
    {4, 5, false},
};

constexpr MotorPins kRightMotors[] = {
    {20, 8, true},
    {9, 10, true},
};

IPAddress kFallbackAccessPointIp(192, 168, 4, 1);
IPAddress kFallbackAccessPointGateway(192, 168, 4, 1);
IPAddress kFallbackAccessPointSubnet(255, 255, 255, 0);

WebSocketsServer webSocketServer(kWebSocketPort);
bool wifiReady = false;
bool timeoutStopReported = false;
uint8_t activeClientCount = 0;

struct MotorState {
  int left = 0;
  int right = 0;
  uint32_t lastCommandAtMs = 0;
} motorState;

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

int clampCommand(int value) {
  return constrain(value, -100, 100);
}

int normalizeCommand(int value) {
  const int clamped = clampCommand(value);
  return abs(clamped) <= kCommandDeadzone ? 0 : clamped;
}

void setDriverSleep(bool enabled) {
  if (kSleepPin == 255) {
    return;
  }

  digitalWrite(kSleepPin, enabled ? HIGH : LOW);
}

void setActivityLed(bool enabled) {
  if (kActivityLedPin == 255) {
    return;
  }

  digitalWrite(kActivityLedPin, enabled ? HIGH : LOW);
}

void stopMotorPins(uint8_t in1Pin, uint8_t in2Pin) {
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
}

void applyMotorPins(uint8_t in1Pin, uint8_t in2Pin, int command) {
  if (command > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    return;
  }

  if (command < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    return;
  }

  stopMotorPins(in1Pin, in2Pin);
}

void stopMotor(const MotorPins &motor) {
  stopMotorPins(motor.in1Pin, motor.in2Pin);
}

void applyMotor(const MotorPins &motor, int command) {
  const int adjustedCommand = motor.invertDirection ? -command : command;
  applyMotorPins(motor.in1Pin, motor.in2Pin, adjustedCommand);
}

template <size_t N>
void applyMotorBank(const MotorPins (&motors)[N], int command) {
  for (const MotorPins &motor : motors) {
    applyMotor(motor, command);
  }
}

void applyLeftMotors(int command) {
  applyMotorBank(kLeftMotors, command);
}

void applyRightMotors(int command) {
  applyMotorBank(kRightMotors, command);
}

String buildStatusJson(const char *reason = nullptr) {
  JsonDocument response;
  response["type"] = "status";
  response["connected"] = activeClientCount > 0;
  response["left"] = motorState.left;
  response["right"] = motorState.right;
  if (reason != nullptr) {
    response["reason"] = reason;
  }

  String message;
  serializeJson(response, message);
  return message;
}

String buildErrorJson(const char *messageText) {
  JsonDocument response;
  response["type"] = "error";
  response["message"] = messageText;

  String message;
  serializeJson(response, message);
  return message;
}

void broadcastStatus(const char *reason = nullptr) {
  String message = buildStatusJson(reason);
  webSocketServer.broadcastTXT(message);
}

void sendStatusToClient(uint8_t clientNumber, const char *reason = nullptr) {
  String message = buildStatusJson(reason);
  webSocketServer.sendTXT(clientNumber, message);
}

void sendErrorToClient(uint8_t clientNumber, const char *messageText) {
  String message = buildErrorJson(messageText);
  webSocketServer.sendTXT(clientNumber, message);
}

void stopAllMotors(const char *reason) {
  motorState.left = 0;
  motorState.right = 0;
  applyLeftMotors(0);
  applyRightMotors(0);
  setDriverSleep(false);
  setActivityLed(false);
  timeoutStopReported = false;
  Serial.printf("[motor] stop reason=%s\n", reason);
}

void applyDriveCommand(int left, int right, const char *source) {
  motorState.left = normalizeCommand(left);
  motorState.right = normalizeCommand(right);
  motorState.lastCommandAtMs = millis();
  timeoutStopReported = false;

  const bool anyMotorActive = motorState.left != 0 || motorState.right != 0;
  setDriverSleep(anyMotorActive);
  setActivityLed(anyMotorActive);
  applyLeftMotors(motorState.left);
  applyRightMotors(motorState.right);

  Serial.printf("[motor] %s left=%d right=%d sleep=%s\n",
                source,
                motorState.left,
                motorState.right,
                anyMotorActive ? "HIGH" : "LOW");
}

template <size_t N>
void configureMotorBank(const MotorPins (&motors)[N]) {
  for (const MotorPins &motor : motors) {
    pinMode(motor.in1Pin, OUTPUT);
    pinMode(motor.in2Pin, OUTPUT);
    stopMotor(motor);
  }
}

void configureMotorPins() {
  configureMotorBank(kLeftMotors);
  configureMotorBank(kRightMotors);

  if (kActivityLedPin != 255) {
    pinMode(kActivityLedPin, OUTPUT);
    digitalWrite(kActivityLedPin, LOW);
  }

  if (kSleepPin != 255) {
    pinMode(kSleepPin, OUTPUT);
    digitalWrite(kSleepPin, LOW);
  }

  motorState.left = 0;
  motorState.right = 0;
  motorState.lastCommandAtMs = 0;
  timeoutStopReported = false;
}

void runBootMotorTest() {
  Serial.println("[boot] Running startup motor spin");
  applyDriveCommand(kBootMotorSpinCommand, kBootMotorSpinCommand, "boot-spin");
  delay(kBootMotorSpinMs);
  stopAllMotors("boot-spin-complete");
  Serial.println("[boot] Startup motor spin complete");
}

void printHelp(Stream &stream) {
  stream.println("Commands:");
  stream.println("  ping");
  stream.println("  wifi status");
  stream.println("  drive <left:-100..100> <right:-100..100>");
  stream.println("  stop");
  stream.println("  help");
}

void reportWifiStatus(Stream &stream, const char *channelName) {
  if (!wifiReady) {
    stream.printf("[%s] wifi not ready\n", channelName);
    return;
  }

  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    stream.printf("[%s] wifi station ssid=%s ip=%s ws_port=%u\n",
                  channelName,
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  kWebSocketPort);
    return;
  }

  stream.printf("[%s] wifi ap ssid=%s ip=%s ws_port=%u\n",
                channelName,
                kFallbackAccessPointSsid,
                WiFi.softAPIP().toString().c_str(),
                kWebSocketPort);
}

bool parseDriveLine(const String &message, int &left, int &right) {
  if (!message.startsWith("drive ")) {
    return false;
  }

  const int parsedCount = sscanf(message.c_str(), "drive %d %d", &left, &right);
  return parsedCount == 2;
}

void handleSerialMessage(const String &message, Stream &replyStream, const char *channelName) {
  if (message.isEmpty()) {
    return;
  }

  if (message == "ping") {
    replyStream.printf("[%s] pong\n", channelName);
    return;
  }

  if (message == "wifi status") {
    reportWifiStatus(replyStream, channelName);
    return;
  }

  if (message == "help") {
    printHelp(replyStream);
    return;
  }

  if (message == "stop") {
    stopAllMotors("serial stop");
    replyStream.printf("[%s] %s\n", channelName, buildStatusJson("serial-stop").c_str());
    broadcastStatus("serial-stop");
    return;
  }

  int left = 0;
  int right = 0;
  if (parseDriveLine(message, left, right)) {
    applyDriveCommand(left, right, "serial");
    replyStream.printf("[%s] %s\n", channelName, buildStatusJson("serial-drive").c_str());
    broadcastStatus("serial-drive");
    return;
  }

  stopAllMotors("serial invalid");
  replyStream.printf("[%s] %s\n", channelName, buildErrorJson("invalid serial command").c_str());
  broadcastStatus("invalid-command");
}

void startWebSocketServer() {
  webSocketServer.begin();
  webSocketServer.enableHeartbeat(30000, 10000, 2);
  Serial.printf("[boot] WebSocket server listening on port %u\n", kWebSocketPort);
}

void startWifiAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  const bool configOk =
      WiFi.softAPConfig(kFallbackAccessPointIp, kFallbackAccessPointGateway, kFallbackAccessPointSubnet);
  const bool started = WiFi.softAP(kFallbackAccessPointSsid, nullptr, kFallbackAccessPointChannel, 0, 2);
  if (!configOk) {
    Serial.println("[boot] Failed to configure Wi-Fi access point IP.");
  }
  if (!started) {
    Serial.println("[boot] Failed to start Wi-Fi access point.");
    return;
  }

  wifiReady = true;
  startWebSocketServer();

  Serial.printf("[boot] Fallback Wi-Fi AP ready. SSID=%s security=open channel=%u\n",
                kFallbackAccessPointSsid,
                kFallbackAccessPointChannel);
  Serial.printf("[boot] Open WebSocket ws://%s:%u/\n",
                WiFi.softAPIP().toString().c_str(),
                kWebSocketPort);
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

  wifiReady = true;
  startWebSocketServer();

  Serial.printf("[boot] Wi-Fi station connected. SSID=%s ip=%s ws_port=%u\n",
                kWifiSsid,
                WiFi.localIP().toString().c_str(),
                kWebSocketPort);
}

bool handleWebSocketJson(const String &message, uint8_t clientNumber) {
  JsonDocument payload;
  const DeserializationError error = deserializeJson(payload, message);
  if (error) {
    sendErrorToClient(clientNumber, "invalid json");
    stopAllMotors("invalid json");
    broadcastStatus("invalid-json");
    return false;
  }

  const char *type = payload["type"];
  if (type == nullptr) {
    sendErrorToClient(clientNumber, "missing type");
    stopAllMotors("missing type");
    broadcastStatus("invalid-command");
    return false;
  }

  if (strcmp(type, "drive") == 0) {
    if (!payload["left"].is<int>() || !payload["right"].is<int>()) {
      sendErrorToClient(clientNumber, "drive requires integer left/right");
      stopAllMotors("invalid drive payload");
      broadcastStatus("invalid-drive");
      return false;
    }

    applyDriveCommand(payload["left"].as<int>(), payload["right"].as<int>(), "websocket");
    broadcastStatus("drive");
    return true;
  }

  if (strcmp(type, "stop") == 0) {
    stopAllMotors("websocket stop");
    broadcastStatus("stop");
    return true;
  }

  if (strcmp(type, "ping") == 0) {
    sendStatusToClient(clientNumber, "pong");
    return true;
  }

  sendErrorToClient(clientNumber, "unknown command type");
  stopAllMotors("unknown command");
  broadcastStatus("invalid-command");
  return false;
}

void handleWebSocketEvent(uint8_t clientNumber, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      activeClientCount++;
      IPAddress ip = webSocketServer.remoteIP(clientNumber);
      Serial.printf("[wifi] websocket client %u connected from %s active=%u\n",
                    clientNumber,
                    ip.toString().c_str(),
                    activeClientCount);
      sendStatusToClient(clientNumber, "connected");
      break;
    }

    case WStype_DISCONNECTED: {
      if (activeClientCount > 0) {
        activeClientCount--;
      }
      Serial.printf("[wifi] websocket client %u disconnected active=%u\n", clientNumber, activeClientCount);
      stopAllMotors("client disconnected");
      broadcastStatus("client-disconnected");
      break;
    }

    case WStype_TEXT: {
      String message;
      message.reserve(length);
      for (size_t index = 0; index < length; ++index) {
        message += static_cast<char>(payload[index]);
      }
      message.trim();
      Serial.printf("[wifi] rx client=%u payload=%s\n", clientNumber, message.c_str());
      handleWebSocketJson(message, clientNumber);
      break;
    }

    default:
      break;
  }
}

void maintainSafetyTimeout() {
  const bool anyMotorActive = motorState.left != 0 || motorState.right != 0;
  if (!anyMotorActive) {
    return;
  }

  if (millis() - motorState.lastCommandAtMs <= kCommandTimeoutMs) {
    return;
  }

  stopAllMotors("command timeout");
  if (!timeoutStopReported) {
    broadcastStatus("timeout");
    timeoutStopReported = true;
  }
}

}  // namespace

void setup() {
  configureMotorPins();
  Serial.begin(kBaudRate);

  const uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
    delay(10);
  }

  Serial.println();
  Serial.println("[boot] XIAO ESP32-C3 motor controller starting");
  Serial.printf("[boot] USB serial ready at %lu baud\n", static_cast<unsigned long>(kBaudRate));

  stopAllMotors("boot");
  runBootMotorTest();
  printHelp(Serial);
  startWifiStation();
  webSocketServer.onEvent(handleWebSocketEvent);
}

void loop() {
  const String serialMessage = readLineFrom(Serial);
  if (serialMessage.length() > 0) {
    handleSerialMessage(serialMessage, Serial, "usb");
  }

  if (wifiReady) {
    webSocketServer.loop();
  }

  maintainSafetyTimeout();
}
