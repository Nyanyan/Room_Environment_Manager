#ifndef SLACK_EVENT_H
#define SLACK_EVENT_H

#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>

// Limit all allocations used by the filtered event and its filter document.
// Oversized retained fields fail cleanly instead of consuming the whole ESP32 heap.
class SlackJsonAllocator : public ArduinoJson::Allocator {
 public:
  static constexpr size_t LIMIT = 16 * 1024;
  void *allocate(size_t size) override {
    if (size > LIMIT - used_ || sizeof(Header) > LIMIT - used_ - size) return nullptr;
    Header *block = static_cast<Header *>(malloc(sizeof(Header) + size));
    if (!block) return nullptr;
    block->size = size;
    used_ += sizeof(Header) + size;
    if (used_ > peak_) peak_ = used_;
    return block + 1;
  }
  void deallocate(void *pointer) override {
    if (!pointer) return;
    Header *block = static_cast<Header *>(pointer) - 1;
    used_ -= sizeof(Header) + block->size;
    free(block);
  }
  void *reallocate(void *pointer, size_t size) override {
    if (!pointer) return allocate(size);
    if (!size) { deallocate(pointer); return nullptr; }
    Header *block = static_cast<Header *>(pointer) - 1;
    const size_t old_size = block->size;
    if (size > old_size && size - old_size > LIMIT - used_) return nullptr;
    Header *resized = static_cast<Header *>(realloc(block, sizeof(Header) + size));
    if (!resized) return nullptr;
    resized->size = size;
    used_ = used_ - old_size + size;
    if (used_ > peak_) peak_ = used_;
    return resized + 1;
  }
  size_t peak() const { return peak_; }
  size_t used() const { return used_; }
 private:
  union Header { size_t size; void *pointer; long double alignment; };
  size_t used_ = 0;
  size_t peak_ = 0;
};

inline void slack_event_filter(JsonDocument &filter) {
  filter["type"] = true;
  filter["envelope_id"] = true;
  filter["retry_attempt"] = true;
  filter["payload"]["event_id"] = true;
  JsonObject event = filter["payload"]["event"].to<JsonObject>();
  for (const char *key : {"type", "subtype", "channel", "bot_id", "bot_profile", "text", "event_ts"}) {
    // Keep only the profile ID, not its images/name/other metadata.
    if (strcmp(key, "bot_profile") == 0) event[key]["id"] = true;
    else event[key] = true;
  }
}

inline bool slack_command_event(JsonDocument &doc, const char *channel) {
  if (strcmp(doc["type"] | "", "events_api") != 0) return false;
  JsonObject event = doc["payload"]["event"];
  const char *text = event["text"] | "";
  return strcmp(event["type"] | "", "message") == 0 &&
         strcmp(event["channel"] | "", channel) == 0 &&
         strlen(event["subtype"] | "") == 0 &&
         strlen(event["bot_id"] | "") == 0 &&
         strlen(event["bot_profile"]["id"] | "") == 0 &&
         text[0] != '\0' && strlen(text) <= 512;
}

#endif
