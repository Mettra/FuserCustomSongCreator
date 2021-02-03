#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "OggMap.h"
#include "oggvorbis.h"

void ComputeMap(vorbis_state* vs, OggMap &map) {
	const uint32_t SEEK_INCREMENT = 0x8000;
	int64_t total_samples = 0;
	std::vector<int64_t> seek_table;
	
	// Record the sample offset for every seek increment, and keep track
	// of the total number of samples in the file.
	uint32_t current_offset = 0;
	for (uint32_t packet_num = 0; vorbis_next(vs) == OK; packet_num++) {
		total_samples = vs->cur_page.granule_pos;
		if (vs->cur_page_start >= current_offset 
		 && vs->cur_packet_start >= current_offset
		 && vs->cur_packet_start >= vs->cur_page_start) {
			seek_table.push_back(vs->next_sample);
			current_offset += SEEK_INCREMENT;
		}
  }

	// Create a map entry of the closest offset for every chunk_size samples in the song.
	int64_t mogg_entries = (total_samples + (map.chunk_size - 1)) / map.chunk_size;
	for (int64_t i = 0; i < mogg_entries; i++) {
		uint32_t desired_position = i * map.chunk_size;
		uint32_t current_bytes = 0;
    uint32_t current_samples = 0;
		for(int j = 0; j < seek_table.size() && seek_table[j] < desired_position; ++j) {
			current_bytes = j * SEEK_INCREMENT;
      current_samples = seek_table[j];
		}
		map.entries.emplace_back(current_bytes, current_samples);
	}
	map.num_entries = map.entries.size();
}


std::variant<std::string, OggMap> OggMap::Create(void* datasource, ov_callbacks callbacks) {
	callbacks.seek_func(datasource, 0, SEEK_SET);
	vorbis_state* vs;
	err e;
	
	if (e = vorbis_init(datasource, &vs, callbacks), e == OK)
	{
		OggMap ret;
		ret.version = 0x10;
		ret.chunk_size = 20000;
		ComputeMap(vs, ret);
		vorbis_free(vs);
		return ret;
	}
	return std::string("Could not init vorbis: ") + str_of_err(e);
}

size_t OggMap::GetLength() {
	return 12 + (entries.size() * 8);
}

// Not endian-safe.
std::vector<char> OggMap::Serialize() {
	std::vector<char> ret;
	ret.resize(GetLength());
	int* ints = (int*)ret.data();
	ints[0] = version;
	ints[1] = chunk_size;
	ints[2] = num_entries;
	for (int i = 0; i < num_entries; i++)
	{
		ints[3 + (i * 2)] = entries[i].bytes;
		ints[3 + (i * 2) + 1] = entries[i].samples;
	}
	return ret;
}