#pragma once

#include <Arduino.h>

namespace fallout_event {

constexpr uint8_t kEspNowChannel = 6;
constexpr uint32_t kProtocolMagic = 0x46414C4C;  // "FALL"
constexpr size_t kMaxTextLength = 64;

struct BlinkCommand {
  uint32_t magic;
  uint32_t sequence;
  uint32_t durationMs;
  char text[kMaxTextLength];
};

inline BlinkCommand makeBlinkCommand(uint32_t sequence, uint32_t durationMs, const String &text) {
  BlinkCommand command = {};
  command.magic = kProtocolMagic;
  command.sequence = sequence;
  command.durationMs = durationMs;
  text.toCharArray(command.text, sizeof(command.text));
  return command;
}

}  // namespace fallout_event
