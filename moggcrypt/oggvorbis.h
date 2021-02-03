#pragma once
#include <stdint.h>
#include <stdio.h>
#ifndef _OV_FILE_H_
#include "XiphTypes.h"
#endif

// This is a barebones implementation of Ogg Vorbis based on the specs at xiph.org
// This doesn't decode PCM samples. It only reads vorbis packets for their blocksize.

enum err;
typedef struct vorbis_state vorbis_state;

// API
// Returns a string constant for the given error code.
const char* str_of_err(err e);
// Initializes a new vorbis_state on the given file.
err vorbis_init(void* datasource, vorbis_state **out, ov_callbacks callbacks);
// Frees the memory allocated by the vorbis_state
void vorbis_free(vorbis_state* s);
// Reads the next audio packet from the stream and updates counters
err vorbis_next(vorbis_state* s);

typedef unsigned char byte;

const size_t MAX_PACKET_SIZE = 0x8000; // Don't deal with packets over this size (512k)

enum err {
	OK = 0,
	READ_ERROR,
	NO_CAPTURE_PATTERN,
	PACKET_TOO_LARGE,
	MALLOC,
	NOT_VORBIS,
	INVALID_DATA,
	INVALID_VERSION,
	INVALID_CHANNELS,
	INVALID_SAMPLE_RATE,
	INVALID_BLOCKSIZE_0,
	INVALID_BLOCKSIZE_1,
	INVALID_CODEBOOK,
	INVALID_MODE,
	INVALID_MAPPING,
	INVALID_FLOOR,
	INVALID_RESIDUES,
	FRAMING_ERROR,
};

typedef struct {
	char capture_pattern[4];
	byte stream_structure_version;
	byte header_type_flag;
	int64_t granule_pos;
	int32_t serial;
	int32_t seq_no;
	int32_t checksum;
	byte page_segments;
	byte segment_table[256];

	long start_pos;
} ogg_page_hdr;

typedef struct {
	byte* buf;
	size_t bitCursor;
	size_t size;
} vorbis_packet;

typedef struct {
	uint32_t vorbis_version;
	uint8_t audio_channels;
	uint32_t audio_sample_rate;
	int32_t bitrate_maximum;
	int32_t bitrate_nominal;
	int32_t bitrate_minimum;
	uint8_t blocksize_0;
	uint8_t blocksize_1;
	uint8_t framing_flag;
} vorbis_id_header;

typedef struct {

} vorbis_comment_header;

typedef struct {
	uint32_t entry_count;
	byte* entries;
	uint8_t lookup_type;

} vorbis_codebook;

typedef struct {

} vorbis_floor;

typedef struct {
	uint32_t begin;
	uint32_t end;
	uint32_t partition_size;
	uint8_t residue_classifications;
	uint8_t residue_classbook;
	uint8_t residue_cascades[64];
} vorbis_residue;

typedef struct {
	uint8_t submaps;
	uint16_t coupling_steps;
} vorbis_mapping;

typedef struct {
	bool blockflag;
	uint16_t windowtype;
	uint16_t transformtype;
	uint8_t mapping;
} vorbis_mode;

typedef struct {
	uint8_t codebook_count;
	vorbis_codebook codebooks[256];
	// time_count
	uint8_t floor_count;
	uint16_t floor_types[64];
	vorbis_floor floor_configurations[64];

	uint8_t residue_count;
	uint16_t residue_types[64];
	vorbis_residue residue_configurations[64];

	uint8_t mapping_count;
	vorbis_mapping mapping_configurations[64];

	uint8_t mode_count;
	vorbis_mode mode_configurations[64];
} vorbis_setup_header;

typedef struct vorbis_state {
	ov_callbacks callbacks;
	void* datasource;
	// The position of the next segment/page within the file
	size_t file_pos;
	// The currently loaded page
	ogg_page_hdr cur_page;
	// Location within the ogg file that the current page starts.
	size_t cur_page_start;
	// Segment within the current page which would be read on the next call to read_packet
	size_t next_segment;
	// The currently loaded packet
	vorbis_packet cur_packet;
	// The PCM sample index (per-channel) that would be the first one returned in the next decode call
	int64_t next_sample;
	uint32_t last_bs;
	// Location within the ogg file that the current packet starts
	size_t cur_packet_start;
	vorbis_id_header id;
	vorbis_setup_header setup;
} vorbis_state;