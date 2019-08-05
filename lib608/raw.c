/*
raw.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <errno.h>
#include <string.h>
#include "608.h"
#include "log.h"

typedef struct {
	char magic[4]; // BCC1/BCC2 (depending on field number)
	u16 bom;
	u8 version_high;
	u8 version_low;
	u32 size;
	u16 header_size;
	u16 section_count; // always 0x0001
	u32 section1_offset;
	u32 section1_size;
	u8 padding[0x28];
} bcc_hdr;

typedef struct {
	char magic[4];
	u32 size;
	u8 padding[8];
} ccdata_hdr;

static const u8* file_header = "\xff\xff\xff\xff";
static const bcc_hdr bcc1_header = {
	"BCC1", 0xfeff, 1, 0, 0, 0, sizeof(bcc_hdr), 1, 0x40, 0
};
static const ccdata_hdr ccd_header = {
	"DATA", 0
};

int WriteRaw(scc_entry* in, size_t* length, FILE* out, f32 fps, timecode start, timecode end) {
	if (in == NULL) {
		log_write(LOG_FATAL, use_colors, "WriteRaw: invalid input pointer\n");
		return 0;
	}
	if (out == NULL) {
		log_write(LOG_ERROR, use_colors, "WriteRaw: invalid file descriptor\n");
		return 0;
	}
	int read_bytes = 0;
	u8* input_ptr = (u8*) in;
	scc_entry* entry = (scc_entry*) input_ptr;
	s64 current_frame = tc2int(start, fps);
	s64 last_frame = tc2int(end, fps);
	s64 next_frame = tc2int(default_timecode, fps);
	s64 first_frame = tc2int(entry->pts.tc, fps);
	log_write(LOG_TRACE, use_colors, "Start: %d End: %d, Current: %d, Next: %d\n", (u32) first_frame, (u32) last_frame, (u32) current_frame, (u32) next_frame);
	if (current_frame > last_frame)  {
		log_write(LOG_WARN, use_colors, "WriteRaw: start > end (adjusting end pts)\n");
		last_frame = current_frame;
	}
	if (first_frame < current_frame) {
		log_write(LOG_WARN, use_colors, "WriteRaw: start pts of input data before specified start time (using pts of first entry)\n");
		current_frame = first_frame;
	}
	int written_bytes = fwrite(file_header, 1, 4, out);
	if (ferror(out)) {
		log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
		return written_bytes;
	}
	u16 padding_bytes = 0;
	while(read_bytes < *length) {
		next_frame = tc2int(entry->pts.tc, fps);
		log_write(LOG_TRACE, use_colors, "WriteRaw: Next Frame %d\n", (s32) next_frame);
		if (next_frame < current_frame) {
			log_write(LOG_ERROR, use_colors, "Timecode %02d:%02hhu:%02hhu%c%02hhu is out of order, or the caption data before it is too big. Aborting.\n", entry->pts.tc.hours, entry->pts.tc.minutes, entry->pts.tc.seconds, entry->pts.tc.drop ? ';' : ':', entry->pts.tc.frames);
			return written_bytes;
		}
		log_write(LOG_TRACE, use_colors, "WriteRaw: 0x8080 padding bytes to write: %d\n", (next_frame - current_frame));
		for (int i = 0; i < (next_frame - current_frame); i++) {
			u16 write = fixParity(padding_bytes);
			written_bytes += fwrite(&write, 2, 1, out);
			if (ferror(out)) {
				log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
				return written_bytes;
			}
		}
		current_frame+=(next_frame - current_frame);
		log_write(LOG_TRACE, use_colors, "WriteRaw: Current Frame %d\n", (s32) current_frame);
		for (int i = 0; i < entry->entry_count; i++) {
			log_write(LOG_TRACE, use_colors, "WriteRaw: CC to encode 0x%04x\n", entry->entries[i]);
			entry->entries[i] = fixParity(entry->entries[i]);
			char bytes[2] = {entry->entries[i] >> 8, entry->entries[i] & 0xff};
			log_write(LOG_TRACE, use_colors, "WriteRaw: Entry address 0x%08x\n", (u32) &(entry->entries[i]));
			u8* byte_pair = (u8*) &(entry->entries[i]);
			byte_pair[0] = bytes[0];
			byte_pair[1] = bytes[1];
			log_write(LOG_TRACE, use_colors, "WriteRaw: Encoded CC 0x%02x%02x\n", byte_pair[0], byte_pair[1]);
		}
		written_bytes += fwrite(entry->entries, 2, entry->entry_count, out);
		if (ferror(out)) {
			log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
			return written_bytes;
		}
		current_frame += entry->entry_count;
		log_write(LOG_TRACE, use_colors, "WriteRaw: Current Frame %d\n", (u32) current_frame);
		read_bytes += sizeof(scc_entry) + (sizeof(u16) * entry->entry_count);
		input_ptr += sizeof(scc_entry) + (sizeof(u16) * entry->entry_count);
		entry = (scc_entry*) input_ptr;
		log_write(LOG_TRACE, use_colors, "WriteRaw: *** input_ptr location 0x%08x ***\n\t*** %d bytes left ***\n", (u32) input_ptr, *length - read_bytes);
	}
	if (last_frame > current_frame) {
		for (int i = 0; i < (last_frame - current_frame); i++) {
			u16 write = fixParity(padding_bytes);
			written_bytes += fwrite(&write, 2, 1, out);
			if (ferror(out)) {
				log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
				return written_bytes;
			}
		}
	}
	else {
		// Write an extra 0x8080 at the end to match McPoodle's tools
		u16 write = fixParity(padding_bytes);
		written_bytes += fwrite(&write, 2, 1, out);
		if (ferror(out)) {
			log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
			return written_bytes;
		}
	}
	log_write(LOG_DEBUG, use_colors, "WriteRaw: wrote %d bytes, from %d bytes of input\n", written_bytes, *length);
	return written_bytes;
}

int WriteNW4R(scc_entry* in, size_t* length, FILE* out, u8 field, bool8 swap) {
	if (in == NULL) {
		log_write(LOG_FATAL, use_colors, "WriteNW4R: invalid input pointer\n");
		return 0;
	}
	if (out == NULL) {
		log_write(LOG_ERROR, use_colors, "WriteNW4R: invalid file descriptor\n");
		return 0;
	}
	field &= 0x1;
	bcc_hdr header = {0};
	memcpy(&header, &bcc1_header, sizeof(bcc_hdr));
	if (field) {
		header.magic[3] = '2';
	}
	header.size = sizeof(bcc_hdr) + sizeof(ccdata_hdr) + *length;
	header.section1_offset = sizeof(bcc_hdr);
	header.section1_size = sizeof(ccdata_hdr) + *length;
	if (swap) {
		header.bom = 0xfffe;
		header.size = byteswap32(header.size);
		header.header_size = byteswap16(header.header_size);
		header.section_count = byteswap16(header.section_count);
		header.section1_offset = byteswap32(header.section1_offset);
		header.section1_size = byteswap32(header.section1_size);
	}
	int written_bytes = fwrite(&header, 1, sizeof(bcc_hdr), out);
	if (ferror(out)) {
		log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
		return written_bytes;
	}
	ccdata_hdr s1hdr = {0};
	memcpy(&s1hdr, &ccd_header, sizeof(ccdata_hdr));
	s1hdr.size = sizeof(ccdata_hdr) + *length;
	if (swap) {
		s1hdr.size = byteswap32(s1hdr.size);
	}
	written_bytes += fwrite(&s1hdr, 1, sizeof(ccdata_hdr), out);
	if (ferror(out)) {
		log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
		return written_bytes;
	}
	if (swap) {
		int read_bytes = 0;
		u8* input_ptr = (u8*) in;
		scc_entry* entry = (scc_entry*) input_ptr;
		int entry_count = 0;
		timecode pts = {0};
		while (read_bytes < *length) {
			entry_count = entry->entry_count;
			entry->entry_count = byteswap32(entry_count);
			pts.hours = byteswap16(entry->pts.tc.hours >> 4);
			pts.minutes = entry->pts.tc.minutes;
			pts.seconds = entry->pts.tc.seconds;
			pts.frames = entry->pts.tc.frames;
			pts.drop = entry->pts.tc.drop;
			entry->pts.tc = pts;
			for (int i=0; i < entry_count; i++) {
				entry->entries[i] = byteswap16(entry->entries[i]);
			}
			read_bytes += sizeof(scc_entry) + (sizeof(u16) * entry_count);
			input_ptr += sizeof(scc_entry) + (sizeof(u16) * entry_count);
			entry = (scc_entry*) input_ptr;
		}
	}
	written_bytes += fwrite(in, 1, *length, out);
	if (ferror(out)) {
		log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
	}
	log_write(LOG_DEBUG, use_colors, "WriteNW4R: wrote %d bytes, from %d bytes of input\n", written_bytes, *length);
	return written_bytes;
}
