#ifndef SLACK_WEBSOCKET_H
#define SLACK_WEBSOCKET_H

#include <stddef.h>
#include <stdint.h>

// The transport callback must enforce an idle timeout and a whole-message deadline.
// Only this reader consumes the socket while a message is being parsed.
class SlackWebSocketReader {
 public:
  typedef bool (*ReadExact)(uint8_t *, size_t);
  typedef bool (*Control)(uint8_t, const uint8_t *, size_t);
  typedef void (*Trace)(uint8_t, bool, uint64_t);
  enum Error { NONE, IO, PROTOCOL, TOO_LARGE, TOO_MANY_FRAMES, CLOSED };
  static constexpr uint32_t MAX_MESSAGE_BYTES = 256UL * 1024UL;

  SlackWebSocketReader(ReadExact read_exact, Control control, Trace trace)
      : read_exact_(read_exact), control_(control), trace_(trace) {}

  // Read one initial frame. Standalone control frames are handled immediately.
  bool begin() {
    if (!header()) return false;
    if (opcode_ >= 0x8) return control_frame();
    if (opcode_ != 0x1 && opcode_ != 0x2) return fail(PROTOCOL);
    text_ = opcode_ == 0x1;
    return account_data();
  }

  int read() {
    if (error_ != NONE) return -1;
    if (buffer_pos_ < buffer_size_) return buffer_[buffer_pos_++];
    while (remaining_ == 0) {
      if (fin_) return -1;
      if (!header()) return -1;
      if (opcode_ >= 0x8) {
        if (!control_frame()) return -1;
        // A control frame does not finish the fragmented data message.
        fin_ = false;
        continue;
      }
      if (opcode_ != 0x0 || !account_data()) {
        if (error_ == NONE) fail(PROTOCOL);
        return -1;
      }
    }
    size_t count = remaining_ < sizeof(buffer_) ? static_cast<size_t>(remaining_) : sizeof(buffer_);
    if (!read_exact_(buffer_, count)) { fail(IO); return -1; }
    remaining_ -= count;
    buffer_pos_ = 1;
    buffer_size_ = count;
    return buffer_[0];
  }

  size_t readBytes(char *destination, size_t length) {
    size_t count = 0;
    for (; count < length; ++count) {
      int value = read();
      if (value < 0) break;
      destination[count] = static_cast<char>(value);
    }
    return count;
  }

  // ArduinoJson stops at the closing brace. Drain the rest of this message,
  // including continuations, without consuming any byte of the next message.
  bool finish(bool whitespace_only = false) {
    bool trailing_ok = true;
    int value;
    while ((value = read()) >= 0) {
      if (whitespace_only && value != ' ' && value != '\t' && value != '\r' && value != '\n') trailing_ok = false;
    }
    if (!trailing_ok && error_ == NONE) fail(PROTOCOL);
    return error_ == NONE;
  }

  bool is_text() const { return text_; }
  Error error() const { return error_; }
  uint32_t message_bytes() const { return total_; }
  const char *error_text() const {
    switch (error_) {
      case NONE: return "none";
      case IO: return "read timeout / disconnected";
      case PROTOCOL: return "invalid WebSocket frame / trailing data";
      case TOO_LARGE: return "message exceeds 256 KiB";
      case TOO_MANY_FRAMES: return "too many fragments / control frames";
      case CLOSED: return "peer closed";
    }
    return "unknown";
  }

 private:
  bool fail(Error error) { error_ = error; return false; }

  bool header() {
    if (++frames_ > 128) return fail(TOO_MANY_FRAMES);
    uint8_t bytes[8];
    if (!read_exact_(bytes, 2)) return fail(IO);
    fin_ = (bytes[0] & 0x80) != 0;
    opcode_ = bytes[0] & 0x0F;
    const bool invalid_flags = (bytes[0] & 0x70) != 0 || (bytes[1] & 0x80) != 0;
    const uint8_t size_code = bytes[1] & 0x7F;
    remaining_ = size_code;
    if (size_code == 126 || size_code == 127) {
      const size_t extra = size_code == 126 ? 2 : 8;
      if (!read_exact_(bytes, extra)) return fail(IO);
      remaining_ = 0;
      for (size_t i = 0; i < extra; ++i) remaining_ = (remaining_ << 8) | bytes[i];
    }
    trace_(opcode_, fin_, remaining_);
    // No compression was negotiated. Server frames must be unmasked (RFC 6455).
    if (invalid_flags || (remaining_ >> 63) != 0 ||
        (size_code == 126 && remaining_ < 126) || (size_code == 127 && remaining_ < 65536)) return fail(PROTOCOL);
    if (opcode_ != 0 && opcode_ != 1 && opcode_ != 2 && opcode_ != 8 && opcode_ != 9 && opcode_ != 10) return fail(PROTOCOL);
    if (opcode_ >= 8 && (!fin_ || remaining_ > 125 || (opcode_ == 8 && remaining_ == 1))) return fail(PROTOCOL);
    return true;
  }

  bool account_data() {
    if (remaining_ > MAX_MESSAGE_BYTES - total_) return fail(TOO_LARGE);
    total_ += static_cast<uint32_t>(remaining_);
    return true;
  }

  bool control_frame() {
    uint8_t payload[125];
    const size_t length = static_cast<size_t>(remaining_);
    if (length && !read_exact_(payload, length)) return fail(IO);
    remaining_ = 0;
    if (opcode_ == 8) return fail(CLOSED);
    if (!control_(opcode_, payload, length)) return fail(IO);
    return true;
  }

  ReadExact read_exact_;
  Control control_;
  Trace trace_;
  uint8_t buffer_[256];
  size_t buffer_pos_ = 0;
  size_t buffer_size_ = 0;
  uint64_t remaining_ = 0;
  uint32_t total_ = 0;
  uint16_t frames_ = 0;
  uint8_t opcode_ = 0;
  bool fin_ = true;
  bool text_ = false;
  Error error_ = NONE;
};

#endif
