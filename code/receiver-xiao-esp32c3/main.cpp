#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace {

constexpr uint32_t kSerialBaud = 115200;
constexpr uint8_t kWifiChannel = 6;
constexpr uint8_t kSenderMac[6] = {0x1C, 0xDB, 0xD4, 0xF1, 0x30, 0x2D};
constexpr char kDummyApSsid[] = "fallout-receiver";

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
volatile bool pingReceived = false;
volatile uint32_t pendingPingId = 0;

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
  if (message.kind != kPing) {
    return;
  }
  pendingPingId = message.id;
  pingReceived = true;
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
  memcpy(peer.peer_addr, kSenderMac, sizeof(kSenderMac));
  peer.channel = 0;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;

  esp_now_del_peer(kSenderMac);
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("esp_now_add_peer failed.");
    return false;
  }

  return true;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(1000);

  Serial.println();
  Serial.println("ESP-NOW receiver starting...");

  WiFi.mode(WIFI_AP_STA);
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
  printMacAddress(kSenderMac);
  Serial.print("Channel: ");
  Serial.println(kWifiChannel);
  Serial.println("Receiver ready.");
}

void loop() {
  if (pingReceived) {
    pingReceived = false;
    Serial.print("Receiver got PING:");
    Serial.println(pendingPingId);

    Message message = {.kind = kEcho, .id = pendingPingId};
    const esp_err_t err =
        esp_now_send(kSenderMac, reinterpret_cast<const uint8_t *>(&message), sizeof(message));
    Serial.print("Echo send queue=");
    Serial.println(err == ESP_OK ? "OK" : "FAIL");
  }

  if (sendDone) {
    sendDone = false;
    Serial.print("Echo send status: ");
    Serial.println(lastSendStatus == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  }
}
