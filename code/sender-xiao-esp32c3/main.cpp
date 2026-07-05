#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace {

constexpr uint32_t kSerialBaud = 115200;
constexpr uint8_t kWifiChannel = 6;
constexpr unsigned long kSendIntervalMs = 1000;
constexpr unsigned long kEchoTimeoutMs = 1500;
constexpr uint8_t kReceiverMac[6] = {0x1C, 0xDB, 0xD4, 0xF1, 0x31, 0x35};
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr char kDummyApSsid[] = "fallout-sender";

enum MessageKind : uint8_t {
  kPing = 1,
  kEcho = 2,
};

struct Message {
  uint8_t kind;
  uint32_t id;
};

volatile bool sendDone = false;
volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;
volatile bool echoReceived = false;
volatile uint32_t echoedId = 0;

unsigned long lastSendAtMs = 0;
unsigned long echoDeadlineMs = 0;
uint32_t nextPingId = 0;
bool waitingForEcho = false;

void printMacAddress(const uint8_t mac[6]) {
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 16) {
      Serial.print('0');
    }
    Serial.print(mac[i], HEX);
    if (i < 5) {
      Serial.print(':');
    }
  }
  Serial.println();
}

void onDataSent(const uint8_t *, esp_now_send_status_t status) {
  lastSendStatus = status;
  sendDone = true;
}

void onDataRecv(const uint8_t *, const uint8_t *incomingData, int len) {
  if (len != static_cast<int>(sizeof(Message))) {
    return;
  }

  Message message = {};
  memcpy(&message, incomingData, sizeof(message));
  if (message.kind == kEcho) {
    echoedId = message.id;
    echoReceived = true;
  }
}

bool initEspNow() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);

  if (!WiFi.softAP(kDummyApSsid, nullptr, kWifiChannel, true, 1)) {
    Serial.println("softAP start failed.");
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init failed.");
    return false;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcastMac, sizeof(kBroadcastMac));
  peer.channel = 0;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;

  esp_now_del_peer(kBroadcastMac);
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("esp_now_add_peer failed.");
    return false;
  }

  return true;
}

void sendPingIfDue() {
  const unsigned long now = millis();
  if (waitingForEcho || now - lastSendAtMs < kSendIntervalMs) {
    return;
  }

  Message message = {.kind = kPing, .id = nextPingId++};
  sendDone = false;
  echoReceived = false;
  waitingForEcho = true;
  echoDeadlineMs = now + kEchoTimeoutMs;
  lastSendAtMs = now;

  const esp_err_t err =
      esp_now_send(kBroadcastMac, reinterpret_cast<const uint8_t *>(&message), sizeof(message));
  Serial.print("Sent PING:");
  Serial.print(message.id);
  Serial.print(" queue=");
  Serial.println(err == ESP_OK ? "OK" : "FAIL");
  if (err != ESP_OK) {
    waitingForEcho = false;
  }
}

void processCallbacks() {
  if (sendDone) {
    sendDone = false;
    Serial.print("Send status: ");
    Serial.println(lastSendStatus == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  }

  if (echoReceived) {
    echoReceived = false;
    Serial.print("Echo OK for ");
    Serial.println(echoedId);
    waitingForEcho = false;
  }
}

void reportTimeoutIfNeeded() {
  if (!waitingForEcho) {
    return;
  }

  if (millis() >= echoDeadlineMs) {
    Serial.print("Echo timeout for ");
    Serial.println(nextPingId - 1);
    waitingForEcho = false;
  }
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(1000);

  Serial.println();
  Serial.println("ESP-NOW sender starting...");
}

void loop() {
  static bool started = false;
  if (!started) {
    started = true;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.setSleep(false);
    delay(100);

    if (!initEspNow()) {
      Serial.println("ESP-NOW setup failed.");
      while (true) {
        delay(1000);
      }
    }
    Serial.print("Local STA MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Local AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("Peer AP MAC: ");
    printMacAddress(kReceiverMac);
    Serial.print("Channel: ");
    Serial.println(kWifiChannel);
    Serial.println("Sender ready.");
  }

  processCallbacks();
  reportTimeoutIfNeeded();
  sendPingIfDue();
}
