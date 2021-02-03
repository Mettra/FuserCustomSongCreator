#pragma once

#include <string>
#include <variant>
#include <vector>

#ifndef _OV_FILE_H_
#include "XiphTypes.h"
#endif

struct OggMap {
  // Create an OggMap from an ogg vorbis file.
  static std::variant<std::string, OggMap> Create(void* datasource, ov_callbacks callbacks);
  // The length in bytes of this when serialized.
  size_t GetLength();
  // Serializes this into a byte array.
  std::vector<char> Serialize();

  uint32_t version;
  uint32_t chunk_size;
  uint32_t num_entries;
  struct Entry {
    Entry(uint32_t bytes, uint32_t samples) : bytes(bytes), samples(samples){}
    uint32_t bytes;
    uint32_t samples;
  };
  std::vector<Entry> entries;
};