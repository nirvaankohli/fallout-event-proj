#pragma once

#include <Arduino.h>

namespace fallout_event {

constexpr uint8_t kEspNowChannel = 1;
constexpr uint32_t kProtocolMagic = 0x46414C4C;  // "FALL"
constexpr size_t kMaxTextLength = 64;

enum PacketKind : uint32_t {
  kPacketKindBlinkCommand = 1,
  kPacketKindBlinkAck = 2,
};

struct BlinkCommand {
  uint32_t magic;
  uint32_t kind;
  uint32_t sequence;
  uint32_t durationMs;
  char text[kMaxTextLength];
};

struct BlinkAck {
  uint32_t magic;
  uint32_t kind;
  uint32_t sequence;
  uint32_t receivedCount;
};

inline BlinkCommand makeBlinkCommand(uint32_t sequence, uint32_t durationMs, const String &text) {
  BlinkCommand command = {};
  command.magic = kProtocolMagic;
  command.kind = kPacketKindBlinkCommand;
  command.sequence = sequence;
  command.durationMs = durationMs;
  text.toCharArray(command.text, sizeof(command.text));
  return command;
}

inline BlinkAck makeBlinkAck(uint32_t sequence, uint32_t receivedCount) {
  BlinkAck ack = {};
  ack.magic = kProtocolMagic;
  ack.kind = kPacketKindBlinkAck;
  ack.sequence = sequence;
  ack.receivedCount = receivedCount;
  return ack;
}

}  // namespace fallout_event
