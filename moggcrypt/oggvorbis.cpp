#include "oggvorbis.h"
#include <stdlib.h>
#include <math.h>

byte ilog(int64_t i)
{
	byte ret = 0;
	while (i > 0) {
		ret++;
		i >>= 1;
	}
	return ret;
}

const char* str_of_err(err e)
{
	switch (e)
	{
	case OK: return "OK";
	case READ_ERROR: return "Read error (probably EOF)";
	case NO_CAPTURE_PATTERN: return "Could not find a capture pattern (OggS)";
	case PACKET_TOO_LARGE: return "Encountered a packet that was too large";
	case MALLOC: return "Out of memory or other malloc failure";
	case NOT_VORBIS: return "Codec in the ogg stream did not identify as vorbis";
	case INVALID_DATA: return "Invalid data was detected";
	case INVALID_VERSION: return "Invalid vorbis version";
	case INVALID_CHANNELS: return "Invalid number of audio channels";
	case INVALID_SAMPLE_RATE: return "Invalid sample rate";
	case INVALID_BLOCKSIZE_0: return "Invalid blocksize 0";
	case INVALID_BLOCKSIZE_1: return "Invalid blocksize 1";
	case INVALID_CODEBOOK: return "Invalid codebook format";
	case INVALID_MODE: return "Invalid mode";
	case INVALID_MAPPING: return "Invalid mapping";
	case INVALID_FLOOR: return "Invalid floor";
	case FRAMING_ERROR: return "Framing error";
	case INVALID_RESIDUES: return "Invalid residues";
	}
	return "Error handling meta-error: invalid error code";
}


// Reads the page header into the 
err page_header_read(vorbis_state* s, ogg_page_hdr* hdr)
{
	void* fi = s->datasource;
	hdr->start_pos = s->callbacks.tell_func(fi);
	hdr->capture_pattern[0] = 0;
	if (s->callbacks.read_func(hdr->capture_pattern, 4, 1, fi) != 1)
		return READ_ERROR;
	if (hdr->capture_pattern[0] != 'O'
		|| hdr->capture_pattern[1] != 'g'
		|| hdr->capture_pattern[2] != 'g'
		|| hdr->capture_pattern[3] != 'S') {
		return NO_CAPTURE_PATTERN;
	}
	if (s->callbacks.read_func(&hdr->stream_structure_version, 1, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->header_type_flag, 1, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->granule_pos, 8, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->serial, 4, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->seq_no, 4, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->checksum, 4, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(&hdr->page_segments, 1, 1, fi) != 1)
		return READ_ERROR;
	if (s->callbacks.read_func(hdr->segment_table, 1, hdr->page_segments, fi) != hdr->page_segments)
		return READ_ERROR;
	return OK;
}

void page_header_print(ogg_page_hdr* hdr)
{
	printf("Capture pattern: %c%c%c%c\n", hdr->capture_pattern[0], hdr->capture_pattern[1], hdr->capture_pattern[2], hdr->capture_pattern[3]);
	printf("Stream structure version: %d\n", hdr->stream_structure_version);
	printf("Header type flag: %x\n", hdr->header_type_flag);
	printf("Granule pos: %lld\n", hdr->granule_pos);
	printf("Serial: %d\n", hdr->serial);
	printf("Sequence number: %d\n", hdr->seq_no);
	printf("Checksum: %x\n", hdr->checksum);
	printf("Page segments: %d\n", hdr->page_segments);
	for (int i = 0; i < hdr->page_segments; i++) {
		printf(" Segment %d: %d\n", i, hdr->segment_table[i]);
	}
}

uint64_t vorbis_read_bits(vorbis_packet* s, size_t count, bool d = false)
{
	if (count > 64 || count == 0) {
		return 0;
	}
	size_t bc = s->bitCursor;
	uint64_t ret = 0;
	uint64_t cur = 0;
	while (count > cur)
	{
		size_t idx = bc >> 3;
		size_t bit = bc & 0x7;

		ret |= ((s->buf[idx] >> bit) & 1ull) << cur;

		bc++;
		cur++;
	}
	s->bitCursor = bc;
	if (s->bitCursor > (s->size << 3))
	{
		printf("Warning: read beyond end of packet\n");
	}
	return ret;
}

err vorbis_read_page(vorbis_state* s)
{
	err e;
	s->cur_page_start = s->callbacks.tell_func(s->datasource);
	if (e = page_header_read(s, &s->cur_page), e != 0)
		return e;
	s->file_pos = s->callbacks.tell_func(s->datasource);
	s->next_segment = 0;
	return OK;
}

// Loads the next packet
err vorbis_read_packet(vorbis_state* s)
{
	err e;
	// The total size of the packet
	size_t packet_size = 0;
	// The amount of the packet that has been read from the file into the buffer
	size_t packet_read = 0;
	// The size of the current segment
	byte segment_length;
	// Read the lacing values until the stop condition (<255 length)
	do
	{
		// Load the next page if we're out of segments in the current page
		if (s->next_segment >= s->cur_page.page_segments)
		{
			if (packet_size - packet_read > 0)
			{
				s->callbacks.read_func(s->cur_packet.buf + packet_read, 1, packet_size - packet_read, s->datasource);
				packet_read = packet_size;
			}
			if (e = vorbis_read_page(s), e != OK)
				return e;
		}
		// Get the length of the next segment and increment the segment pointer
		segment_length = s->cur_page.segment_table[s->next_segment++];
		if (packet_size == 0) {
			s->cur_packet_start = s->file_pos;
		}
		packet_size += segment_length;
		s->file_pos += segment_length;
		if (packet_size > MAX_PACKET_SIZE)
		{
			// Literally cannot deal with this
			return PACKET_TOO_LARGE;
		}
	} while (segment_length == 255);
	if (packet_size - packet_read > 0)
	{
		s->callbacks.read_func(s->cur_packet.buf + packet_read, 1, packet_size - packet_read, s->datasource);
	}

	s->cur_packet.size = packet_size;
	s->cur_packet.bitCursor = 0;
	return OK;
}

void vorbis_free(vorbis_state* s)
{
	if (s == NULL) return;
	if (s->cur_packet.buf) {
		free(s->cur_packet.buf);
	}
	free(s);
}

const uint64_t VORBIS_ID = 0x736962726f76ull;

// Reads the vorbis ID header packet
err vorbis_read_id(vorbis_state* s)
{
	err e;
	if (e = vorbis_read_packet(s), e != OK)
		return e;
	if (s->cur_packet.size != 30)
		return NOT_VORBIS;
	vorbis_packet* p = &s->cur_packet;
	if (vorbis_read_bits(p, 8) != 1)
		return NOT_VORBIS;
	// 'sibrov' little endian
	uint64_t v;
	if (v = vorbis_read_bits(p, 48), v != VORBIS_ID)
	{
		return NOT_VORBIS;
	}
	s->id.vorbis_version = vorbis_read_bits(p, 32);
	s->id.audio_channels = vorbis_read_bits(p, 8);
	s->id.audio_sample_rate = vorbis_read_bits(p, 32);
	s->id.bitrate_maximum = vorbis_read_bits(p, 32);
	s->id.bitrate_nominal = vorbis_read_bits(p, 32);
	s->id.bitrate_minimum = vorbis_read_bits(p, 32);
	s->id.blocksize_0 = vorbis_read_bits(p, 4);
	s->id.blocksize_1 = vorbis_read_bits(p, 4);
	s->id.framing_flag = vorbis_read_bits(p, 1);

	if (s->id.vorbis_version != 0)
		return INVALID_VERSION;
	if (s->id.audio_channels == 0)
		return INVALID_CHANNELS;
	if (s->id.audio_sample_rate == 0)
		return INVALID_SAMPLE_RATE;
	if (s->id.blocksize_0 < 6 || s->id.blocksize_0 > 13)
		return INVALID_BLOCKSIZE_0;
	if (s->id.blocksize_1 < 6 || s->id.blocksize_1 > 13)
		return INVALID_BLOCKSIZE_1;
	if (s->id.blocksize_0 > s->id.blocksize_1)
		return INVALID_BLOCKSIZE_0;
	return OK;
}

// Reads the vorbis setup header packet
err vorbis_read_setup(vorbis_state *s)
{
	err e;
	if (e = vorbis_read_packet(s), e != OK)
		return e;
	vorbis_packet* p = &s->cur_packet;
	if (vorbis_read_bits(p, 8) != 5)
	{
		return INVALID_DATA;
	}
	if (vorbis_read_bits(p, 48) != VORBIS_ID)
	{
		return INVALID_DATA;
	}

	// Read codebooks
	s->setup.codebook_count = vorbis_read_bits(p, 8) + 1;
	for (auto i = 0; i < s->setup.codebook_count; i++)
	{
		vorbis_codebook *c = &s->setup.codebooks[i];
		if (vorbis_read_bits(p, 24) != 0x564342)
			return INVALID_CODEBOOK;

		uint32_t codebook_dimensions = (uint32_t)vorbis_read_bits(p, 16, true);
		uint32_t codebook_entries = (uint32_t)vorbis_read_bits(p, 24, true);
		c->entries = (byte*)malloc(codebook_entries);
		// !ordered
		if (!vorbis_read_bits(p, 1))
		{
			bool sparse = vorbis_read_bits(p, 1);
			for (int j = 0; j < codebook_entries; j++)
			{
				// sparse
				if (sparse)
				{
					// used flag
					if (vorbis_read_bits(p, 1))
					{
						c->entries[j] = vorbis_read_bits(p, 5) + 1;
					}
					else {
						c->entries[j] = 0;
					}
				}
				else // not sparse
				{
					c->entries[j] = vorbis_read_bits(p, 5) + 1;
				}
			}
		}
		else // ordered
		{
			byte length = vorbis_read_bits(p, 5) + 1;
			for (int j = 0; j != codebook_entries; length++)
			{
				byte number = vorbis_read_bits(p, ilog(codebook_entries - j));
				for (int k = j; k < (j + number); k++)
					c->entries[k] = length;
				j += number;
				if (j > codebook_entries)
					return INVALID_CODEBOOK;
			}
		}

		c->lookup_type = vorbis_read_bits(p, 4);
		if (c->lookup_type > 2)
			return INVALID_CODEBOOK;
		else if (c->lookup_type > 0)
		{
			// codebook_minimum_value
			vorbis_read_bits(p, 32);
			// codebook_delta_value
			vorbis_read_bits(p, 32);
			byte value_bits = vorbis_read_bits(p, 4) + 1;
			// codebook_sequence_p
			vorbis_read_bits(p, 1);
			uint64_t lookup_values = 0;
			if (c->lookup_type == 1)
			{
				while (pow(lookup_values, codebook_dimensions) <= codebook_entries)
				{
					lookup_values++;
				}
				if (lookup_values > 0)
					lookup_values--;

			}
			else
			{
				lookup_values = codebook_entries * codebook_dimensions;
			}

			for (uint64_t j = 0; j < lookup_values; j++)
			{
				vorbis_read_bits(p, value_bits);
			}
		}
	}

	// Read (skip) time domain transforms
	auto time_count = vorbis_read_bits(p, 6) + 1;
	for (auto i = 0ull; i < time_count; i++)
	{
		if (vorbis_read_bits(p, 16) != 0)
			return INVALID_DATA;
	}

	// Read floor
	s->setup.floor_count = vorbis_read_bits(p, 6) + 1;
	for (auto i = 0; i < s->setup.floor_count; i++)
	{
		uint16_t floor_type = vorbis_read_bits(p, 16);
		if (floor_type == 0)
		{
			// floor0_order
			vorbis_read_bits(p, 8);
			// floor0_rate
			vorbis_read_bits(p, 16);
			// floor0_bark_map_size
			vorbis_read_bits(p, 16);
			// floor0_amplitude_bits
			vorbis_read_bits(p, 6);
			// floor0_amplitude_offset
			vorbis_read_bits(p, 8);
			byte number_of_books = vorbis_read_bits(p, 4) + 1;
			// [floor0_book_list] = read a list of [floor0_number_of_books] unsigned integers of eight bits each;
			for (int i = 0; i < number_of_books; i++)
			{
				// floor0_book_list[i]
				vorbis_read_bits(p, 8);
			}
		}
		else if (floor_type == 1)
		{
			byte class_list[32];
			byte dimensions[16];
			byte subclasses[16];
			byte masterbooks[16];

			byte partitions = vorbis_read_bits(p, 5);
			int max_class = -1;
			for (auto j = 0; j < partitions; j++)
			{
				// floor1_partition_class_list[j]
				class_list[j] = vorbis_read_bits(p, 4);
				if (class_list[j] > max_class)
					max_class = class_list[j];
			} 
			for (auto j = 0; j <= max_class; j++)
			{
				dimensions[j] = vorbis_read_bits(p, 3) + 1;
				subclasses[j] = vorbis_read_bits(p, 2);
				if (subclasses[j])
				{
					masterbooks[j] = vorbis_read_bits(p, 8);
					if (masterbooks[j] >= s->setup.codebook_count)
						return INVALID_FLOOR;
				}
				int exp = 1 << subclasses[j];

				for (auto k = 0; k < exp; k++)
				{
					// floor1_subclass_books[j][k]
					int16_t subclass_books = (int16_t)vorbis_read_bits(p, 8) - 1;
					if (subclass_books >= s->setup.codebook_count)
						return INVALID_FLOOR;
				}
			}
			
			// floor1_multiplier
			vorbis_read_bits(p, 2);
			byte rangebits = vorbis_read_bits(p, 4);
			byte floor1_values = 2;
			for (auto j = 0; j < partitions; j++)
			{
				byte classnum = class_list[j];

				for (auto k = 0; k < dimensions[classnum]; k++)
				{
					vorbis_read_bits(p, rangebits);
					floor1_values++;
					if (floor1_values > 64)
						return INVALID_FLOOR;
				}
			}
		}
		else
		{
			return INVALID_FLOOR;
		}
	}

	// Read residues
	s->setup.residue_count = vorbis_read_bits(p, 6) + 1;
	for (auto i = 0; i < s->setup.residue_count; i++)
	{
		vorbis_residue *r = &s->setup.residue_configurations[i];
		uint16_t residue_types = vorbis_read_bits(p, 16);
		if (residue_types > 2)
			return INVALID_RESIDUES;
		r->begin = vorbis_read_bits(p, 24);
		r->end = vorbis_read_bits(p, 24);
		r->partition_size = vorbis_read_bits(p, 24) + 1;
		r->residue_classifications = vorbis_read_bits(p, 6) + 1;
		r->residue_classbook = vorbis_read_bits(p, 8);
		for (int j = 0; j < r->residue_classifications; j++)
		{
			int high_bits = 0;
			int low_bits = vorbis_read_bits(p, 3);
			if (vorbis_read_bits(p, 1))
			{
				high_bits = vorbis_read_bits(p, 5);
			}
			r->residue_cascades[j] = (high_bits << 3) | low_bits;
		}
		for (int j = 0; j < r->residue_classifications; j++)
		{
			for (int k = 0; k < 8; k++)
			{
				if (r->residue_cascades[j] & (1 << k))
				{
					// residue_books[j][k]
					vorbis_read_bits(p, 8);
				}
			}
		}
	}

	// Read mappings
	s->setup.mapping_count = vorbis_read_bits(p, 6) + 1;
	for (auto i = 0; i < s->setup.mapping_count; i++)
	{
		vorbis_mapping* m = &s->setup.mapping_configurations[i];
		// Check that mapping is type 0
		if (vorbis_read_bits(p, 16) != 0)
			return INVALID_MAPPING;
		if (vorbis_read_bits(p, 1))
		{
			m->submaps = vorbis_read_bits(p, 4) + 1;
		}
		else
		{
			m->submaps = 1;
		}

		if (vorbis_read_bits(p, 1))
		{
			m->coupling_steps = vorbis_read_bits(p, 8) + 1;
			for (int j = 0; j < m->coupling_steps; j++)
			{
				// vorbis_mapping_magnitude
				vorbis_read_bits(p, ilog(s->id.audio_channels - 1));
				// vorbis_mapping_angle
				vorbis_read_bits(p, ilog(s->id.audio_channels - 1));
			}
		}
		else
		{
			m->coupling_steps = 0;
		}

		if (vorbis_read_bits(p, 2) != 0)
			return INVALID_MAPPING;
		if (m->submaps > 1)
		{
			for (int j = 0; j < s->id.audio_channels; j++)
			{
				// vorbis_mapping_mux
				if (vorbis_read_bits(p, 4) > m->submaps)
					return INVALID_MAPPING;
			}
		}

		for (int j = 0; j < m->submaps; j++)
		{
			// time configuration placeholder
			vorbis_read_bits(p, 8);
			// floor number
			if (vorbis_read_bits(p, 8) > s->setup.floor_count)
				return INVALID_MAPPING;
			// residue number
			if (vorbis_read_bits(p, 8) > s->setup.residue_count)
				return INVALID_MAPPING;
		}
	}

	// Read modes
	s->setup.mode_count = vorbis_read_bits(p, 6) + 1;
	vorbis_mode* modes = s->setup.mode_configurations;
	for (auto i = 0; i < s->setup.mode_count; i++)
	{
		modes[i].blockflag = vorbis_read_bits(p, 1);
		modes[i].windowtype = vorbis_read_bits(p, 16);
		modes[i].transformtype = vorbis_read_bits(p, 16);
		modes[i].mapping = vorbis_read_bits(p, 8);
		if (modes[i].windowtype != 0
			|| modes[i].transformtype != 0
			|| modes[i].mapping >= s->setup.mapping_count)
			return INVALID_MODE;
	}
	if (vorbis_read_bits(p, 1) == 0)
		return FRAMING_ERROR;

	return OK;
}

// Initializes a new vorbis_state on the given file.
// Loads the header packets in order to initialize the structure.
// Thus the next call to vorbis_read_packet will give you the first audio packet.
err vorbis_init(void* datasource, vorbis_state **out, ov_callbacks callbacks)
{
	err e;
	vorbis_state *s = (vorbis_state*)calloc(sizeof(vorbis_state), 1);
	if (!s) {
		e = MALLOC;
		goto fail;
	}
	s->callbacks = callbacks;
	s->datasource = datasource;
	s->next_sample = 0;
	s->file_pos = 0;
	s->last_bs = 0;
	s->cur_packet.buf = (byte*)malloc(MAX_PACKET_SIZE);
	if (!s->cur_packet.buf)
	{
		e = MALLOC;
		goto fail;
	}
	s->cur_packet.size = 0;
	if (e = vorbis_read_page(s), e != OK)
		goto fail;

	// Read the ID header
	if (e = vorbis_read_id(s), e != OK)
		goto fail;

	// Read comment header
	if (e = vorbis_read_packet(s), e != OK)
		goto fail;
	if (vorbis_read_bits(&s->cur_packet, 8) != 3)
	{
		e = INVALID_DATA;
		goto fail;
	}

	// Read setup header
	if (e = vorbis_read_setup(s), e != OK)
		goto fail;

	*out = s;
	return OK;

	// To reduce repetition/indentation depth, this frees the vorbis state and returns the last error
fail:
	vorbis_free(s);
	*out = NULL;
	return e;
}

// Reads the next audio packet from the stream and updates counters
err vorbis_next(vorbis_state* vb)
{
	err e;
	if (e = vorbis_read_packet(vb), e != OK)
		return e;
	vorbis_packet *p = &vb->cur_packet;

	// Audio LSB is 0
	if (vorbis_read_bits(p, 1) != 0)
		return INVALID_DATA;
	uint32_t mode_number = vorbis_read_bits(p, ilog(vb->setup.mode_count - 1));
	uint32_t blocksize = vb->setup.mode_configurations[mode_number].blockflag
		? vb->id.blocksize_1 : vb->id.blocksize_0;
	blocksize = 1 << blocksize; // blocksize is a 2-exponent
	if (vb->last_bs)
		vb->next_sample += (vb->last_bs + blocksize) / 4;
	vb->last_bs = blocksize;
	
	return OK;
}