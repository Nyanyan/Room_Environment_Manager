#include <cassert>
#include <cstring>
#include <iostream>
#include "Arduino.h"
#include "slack_event.h"
#include "slack_websocket.h"

struct FakeSocket {
  std::vector<uint8_t> input, output;
  size_t position = 0;
  size_t chunk = 17;
  uint32_t read_delay = 0;
  bool live = true;
  bool write_ok = true;
  bool connected() { return live; }
  int available() { return live ? static_cast<int>(std::min(chunk, input.size() - position)) : 0; }
  int read(uint8_t *data, size_t length) {
    size_t count = std::min(length, static_cast<size_t>(available()));
    memcpy(data, input.data() + position, count);
    position += count;
    fake_now += read_delay;
    return static_cast<int>(count);
  }
  size_t write(const uint8_t *data, size_t length) {
    if (!live || !write_ok) return 0;
    output.insert(output.end(), data, data + length);
    return length;
  }
  void stop() { live = false; }
};
FakeSocket g_slack_socket;
bool g_slack_socket_started = true;
unsigned long g_next_socket_connect_ms = 0;
unsigned long g_last_socket_rx_ms = 0;
String g_last_slack_event_ts;
#define SLACK_CHANNEL_ID "C_TEST"
// Generated from the real .ino by run_tests.py. No copy of the implementation.
#include "socket_under_test.inc"

void reset() {
  g_slack_socket = FakeSocket();
  g_slack_socket_started = true;
  g_slack_command_head = 0; g_slack_command_count = 0;
  g_last_slack_event_ts = "";
  fake_now = 0;
  Serial.lines.clear();
}

std::vector<uint8_t> frame(const std::string &payload, uint8_t opcode = 1, bool fin = true) {
  std::vector<uint8_t> result{static_cast<uint8_t>((fin ? 0x80 : 0) | opcode)};
  size_t size = payload.size();
  if (size <= 125) result.push_back(static_cast<uint8_t>(size));
  else if (size <= 65535) {
    result.push_back(126); result.push_back(size >> 8); result.push_back(size);
  } else {
    result.push_back(127);
    for (int i = 7; i >= 0; --i) result.push_back(static_cast<uint64_t>(size) >> (i * 8));
  }
  result.insert(result.end(), payload.begin(), payload.end());
  return result;
}
void push(const std::vector<uint8_t> &data) {
  g_slack_socket.input.insert(g_slack_socket.input.end(), data.begin(), data.end());
}

std::string event(const std::string &text = "monitor", const std::string &extra = "", const std::string &metadata = "") {
  return "{\"type\":\"events_api\",\"payload\":{\"event_id\":\"Ev1\",\"event\":{"
         "\"type\":\"message\",\"channel\":\"C_TEST\",\"event_ts\":\"123.456\",\"text\":\"" + text + "\"" + extra +
         "},\"metadata\":\"" + metadata + "\"},\"envelope_id\":\"env-1\"}";
}

struct SentFrame { uint8_t opcode; std::string payload; };
std::vector<SentFrame> sent() {
  std::vector<SentFrame> result;
  const auto &bytes = g_slack_socket.output;
  size_t position = 0;
  while (position < bytes.size()) {
    uint8_t opcode = bytes.at(position++) & 15;
    uint8_t size_code = bytes.at(position++);
    assert(size_code & 0x80); // every client frame must be masked
    size_t length = size_code & 127;
    if (length == 126) { length = bytes.at(position) * 256 + bytes.at(position + 1); position += 2; }
    assert(length != 127);
    uint8_t mask[4];
    for (int i = 0; i < 4; ++i) mask[i] = bytes.at(position++);
    std::string payload;
    for (size_t i = 0; i < length; ++i) payload += static_cast<char>(bytes.at(position++) ^ mask[i % 4]);
    result.push_back({opcode, payload});
  }
  return result;
}
int ack_count() {
  int result = 0;
  for (const auto &message : sent()) {
    if (message.opcode == 1) {
      JsonDocument doc;
      assert(!deserializeJson(doc, message.payload));
      assert(strcmp(doc["envelope_id"] | "", "env-1") == 0);
      ++result;
    }
  }
  return result;
}
bool logged(const char *text) {
  for (const auto &line : Serial.lines) if (line.find(text) != std::string::npos) return true;
  return false;
}

int main() {
  // More than 8 KiB, and also more than 64 KiB. Envelope ID is deliberately LAST.
  // Large metadata must not prevent ACK or eat the following frame.
  reset();
  push(frame(event("hm on weak", "", std::string(180000, 'x'))));
  size_t boundary = g_slack_socket.input.size();
  push(frame(event("monitor")));
  assert(read_slack_socket_frame());
  assert(g_slack_socket.position == boundary && ack_count() == 1);
  assert(dequeue_slack_command() == "hm on weak");
  assert(logged("opcode:0x01 fin:1") && logged("JSON peak:"));
  assert(read_slack_socket_frame() && ack_count() == 2);
  assert(g_slack_command_count == 0); // duplicate timestamp ignored, still ACKed

  // Large attachments/nested previous messages and bots without a subtype.
  for (const auto &extra : {
      std::string(",\"subtype\":\"message_changed\",\"previous_message\":{\"text\":\"") + std::string(100000, 'x') + "\"}",
      std::string(",\"bot_id\":\"B1\",\"attachments\":[{\"text\":\"") + std::string(100000, 'x') + "\"}]",
      std::string(",\"bot_profile\":{\"id\":\"B1\",\"icons\":{\"image\":\"") + std::string(100000, 'x') + "\"}}",
      std::string(",\"channel\":\"OTHER\"")}) {
    reset(); push(frame(event("hm on strong", extra)));
    assert(read_slack_socket_frame() && ack_count() == 1 && g_slack_command_count == 0);
  }
  reset(); push(frame("{\"type\":\"interactive\",\"envelope_id\":\"env-1\"}"));
  assert(read_slack_socket_frame() && ack_count() == 1 && g_slack_command_count == 0);

  // Fragment across a JSON escape and interleave ping/pong control frames.
  reset();
  std::string payload = event("hm on \\u0077eak");
  size_t split = payload.find("\\u0077") + 3;
  push(frame(payload.substr(0, split), 1, false));
  push(frame("ping-data", 9));
  push(frame("", 10));
  push(frame(payload.substr(split, 10), 0, false));
  push(frame(payload.substr(split + 10), 0, true));
  assert(read_slack_socket_frame() && ack_count() == 1);
  assert(dequeue_slack_command() == "hm on weak");
  assert(sent()[0].opcode == 10 && sent()[0].payload == "ping-data");
  assert(logged("opcode:0x00 fin:1"));

  // Standalone controls, binary messages, trailing whitespace and early parser exit.
  reset(); push(frame("hello", 9)); push(frame("binary", 2)); push(frame(event() + " \r\n\t"));
  assert(read_slack_socket_frame() && sent()[0].payload == "hello");
  assert(read_slack_socket_frame() && g_slack_command_count == 0);
  assert(read_slack_socket_frame() && dequeue_slack_command() == "monitor");
  reset(); push(frame(event() + " unexpected"));
  assert(!read_slack_socket_frame() && !g_slack_socket.connected() && g_slack_command_count == 0);
  reset(); push(frame("{\"envelope_id\":\"env-1\",BROKEN}")); push(frame(event()));
  assert(read_slack_socket_frame() && g_slack_command_count == 0);
  assert(read_slack_socket_frame() && dequeue_slack_command() == "monitor");

  // A huge retained field is bounded; reject it, ACK when envelope is known,
  // and preserve the connection and framing for the next command.
  reset();
  push(frame("{\"envelope_id\":\"env-1\",\"type\":\"events_api\",\"payload\":{\"event\":{\"text\":\"" + std::string(50000, 'a') + "\"}}}"));
  push(frame(event()));
  assert(read_slack_socket_frame() && ack_count() == 1 && logged("NoMemory"));
  assert(read_slack_socket_frame() && dequeue_slack_command() == "monitor");
  reset(); push(frame(event(std::string(513, 'a'))));
  assert(read_slack_socket_frame() && ack_count() == 1 && g_slack_command_count == 0);

  // Do not execute a command when ACK cannot be transmitted.
  reset(); push(frame(event())); g_slack_socket.write_ok = false;
  assert(!read_slack_socket_frame() && !g_slack_socket.connected() && g_slack_command_count == 0);

  // Protocol errors cannot leave the parser in the middle of a frame.
  for (const auto &bad : {
      std::vector<uint8_t>{0x81, 0x80}, // masked server frame
      std::vector<uint8_t>{0xC1, 0}, // RSV without negotiated extension
      std::vector<uint8_t>{0x80, 0}, // unexpected continuation
      std::vector<uint8_t>{0x09, 0}, // fragmented ping
      std::vector<uint8_t>{0x89, 126, 0, 126}, // oversized control
      std::vector<uint8_t>{0x81, 126, 0, 2}, // nonminimal length
      std::vector<uint8_t>{0x81, 127, 0x80, 0, 0, 0, 0, 0, 0, 0},
      std::vector<uint8_t>{0x81, 127, 0, 0, 0, 0, 0, 4, 0, 1}}) { // >256 KiB
    reset(); push(bad);
    assert(!read_slack_socket_frame() && !g_slack_socket.connected());
  }
  reset(); push(frame("{", 1, false)); push(frame("}", 1, true));
  assert(!read_slack_socket_frame());
  reset(); push(frame("{\"unused\":\"" + std::string(180000, 'x'), 1, false));
  push(frame(std::string(100000, 'x') + "\"}", 0, true));
  assert(!read_slack_socket_frame() && logged("message exceeds 256 KiB"));
  reset(); push(frame("", 1, false));
  for (int i = 0; i < 128; ++i) push(frame("", 0, false));
  assert(!read_slack_socket_frame() && logged("too many fragments"));
  reset(); push(frame("{", 1, false)); push(frame("", 8));
  assert(!read_slack_socket_frame() && logged("peer closed"));

  // Missing continuation or interrupted TCP stream is bounded by the idle timeout.
  reset(); push({0x81, 20, '{'});
  assert(!read_slack_socket_frame() && fake_now >= 1000 && fake_now <= SLACK_MESSAGE_TIMEOUT_MS);
  reset(); push(frame(event("monitor", "", std::string(10000, 'x'))));
  g_slack_socket.chunk = 2; g_slack_socket.read_delay = 10;
  const bool slow_result = read_slack_socket_frame();
  assert(!slow_result && fake_now >= SLACK_MESSAGE_TIMEOUT_MS && fake_now <= SLACK_MESSAGE_TIMEOUT_MS + 10);
  reset(); fake_now = UINT32_MAX - 20; g_slack_socket.read_delay = 1;
  push(frame(event()));
  assert(read_slack_socket_frame() && ack_count() == 1); // millis() rollover

  // Filtering uses the same bounded heap for small and very large discarded fields.
  size_t small_peak = 0;
  for (size_t size : {size_t(100), size_t(180000)}) {
    SlackJsonAllocator allocator;
    {
      JsonDocument doc(&allocator), filter(&allocator);
      slack_event_filter(filter);
      assert(!deserializeJson(doc, event("monitor", "", std::string(size, 'x')), DeserializationOption::Filter(filter)));
      assert(slack_command_event(doc, SLACK_CHANNEL_ID));
      if (size == 100) small_peak = allocator.peak();
      else assert(allocator.peak() == small_peak);
    }
    assert(allocator.used() == 0 && allocator.peak() <= SlackJsonAllocator::LIMIT);
  }
  std::cout << "Slack socket tests passed: large/fragmented events, bounded memory, ACK/ignore, framing, controls and deadlines\n";
  std::cout << "Filtered JSON peak for 180 KB metadata: " << small_peak << " bytes\n";
}
