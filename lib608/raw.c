/*
raw.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdlib.h>
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
	struct {
		u32 offset;
		u32 size;
	} sections[6];
} bcc_hdr;

typedef struct {
	char magic[4];
	u32 size;
	u8 padding[8];
} ccdata_hdr;

static const char* const file_header = "\xff\xff\xff\xff";
static const bcc_hdr bcc1_header = {
	.magic = "BCC1",
	.bom = 0xfeff,
	.version_high = 1,
	.version_low = 0,
	.header_size = sizeof(bcc_hdr),
	.section_count = 1,
	.sections = {
		[0] = {
			.offset = sizeof(bcc_hdr)
		}
	}
};
static const ccdata_hdr ccd_header = {
	.magic = "DATA"
};

// number of 0x8080's encountered before output of ReadRaw stops
unsigned int MAX_NULLS=2;

scc_entry* ReadRaw(FILE* raw, size_t* length, f32 fps, timecode start, bool8 drop) {
	if (raw == NULL) {
		log_write(LOG_ERROR, use_colors, "ReadRaw: invalid file descriptor\n");
		return NULL;
	}
	u8 check[4];
	if (fread(&check, 1, 4, raw) != 4) {
		// check read error
		if (ferror(raw)) {
			log_write(LOG_ERROR, use_colors, "ReadRaw: Error reading file (%d: %s)\n", errno, strerror(errno));
			return NULL;
		}
		// check eof
		else if (feof(raw)) {
			log_write(LOG_ERROR, use_colors, "ReadRaw: unexpected end of file\n");
			return NULL;
		}
		else { // fread was successful but didn't return expected amount of bytes
			return NULL;
		}
	}
	if (memcmp(check, file_header, 4) != 0) {
		log_write(LOG_ERROR, use_colors, "ReadRaw: Input is not a raw broadcast file\n");
		return NULL;
	}
	size_t allocated = 8192;
	scc_entry* cc_data = malloc(allocated);
	if (cc_data == NULL) {
		log_write(LOG_FATAL, use_colors, "ReadRaw: Memory allocation for output data failed\n");
		return NULL;
	}
	u8* output_ptr = (u8*) cc_data;
	u8* end_ptr = (u8*) cc_data+allocated;
	log_write(LOG_TRACE, use_colors, "ReadRaw: *** Pointer Locations ***\n\toutput_ptr 0x%08x\n\tend_ptr 0x%08x\n\tcc_data 0x%08x\n", (u32) output_ptr, (u32) end_ptr, (u32) cc_data);
	s64 current_frame = tc2int(start, fps)-1; // sub 1 due to loop
	// ftell() = 4, is past the header so go for it!
	u8 read_ccs[2] = {0, 0};
	unsigned int null_cnt = 0;
	unsigned int cc_cnt = 0;
	int record_count = 0;
	int channel = 3; // Assume we're in XDS mode by default
	bool8 output = false;
	bool8 received_cr = false;
	bool8 eol = false; // sets frame count on current record
	while (fread(read_ccs, 1, 2, raw) == 2) {
		current_frame++;
		// Get read_ccs into native byte order
		u16 cc = ((read_ccs[0] << 8) | read_ccs[1]) & 0x7f7f;
		if (output) {
			cc_cnt++;
		}
		if (cc == 0 && !output) {
			continue;
		}
		if (cc != 0) {
			null_cnt = 0;
		}
		if (cc == 0 && output) {
			null_cnt++;
			// Padding will be auto applied due to how pointers work in C, lol
		}
		if (null_cnt > MAX_NULLS) {
			cc_cnt -= (null_cnt-1); // safe to set here as this condition can only be triggered by a null, and the very next check will also unset the output flag. -1 due to 1-based index of cc_cnt
			null_cnt = 0;
			eol = true;
			log_write(LOG_TRACE, use_colors, "ReadRaw: Null count exceeds %d, setting eol\n", MAX_NULLS);
		}
		if (cc == 0 && eol) {
			eol = false;
			output = false;
			log_write(LOG_TRACE, use_colors, "ReadRaw: Stopping output\n");
		}
		// Check for a repeat CR code (bit 9: channel, bit 12: field)
		if (!((cc | 0x900) == 0x1d2d) && received_cr) {
			eol = false;
			output = false;
			log_write(LOG_TRACE, use_colors, "ReadRaw: Stopping output\n");
		}
		bool8 isControlCode = (cc | 0x90f) == 0x1d2f;
		bool8 isXDS = (cc | 0xf7f) == 0xf7f;
		if (isControlCode || isXDS) {
			u16 control_check = cc & 0x2f;
			u16 xds_check = (cc & 0xf00) >> 8;
			bool8 isValidXDSCode = isXDS && (xds_check > 0) && (xds_check <= 0xf);
			if ((isControlCode && ((control_check == 0x20) || (control_check == 0x25) || (control_check == 0x26) || (control_check == 0x27) || (control_check == 0x29) || (control_check == 0x2a) || (control_check == 0x2b))) || (isValidXDSCode && xds_check != 0xf)) {
				log_write(LOG_TRACE, use_colors, "ReadRaw: XDS or control code recieved.\n");
				int check_channel = 1;
				bool8 field = (cc & 0x100) >> 8;
				bool8 bchannel = (cc & 0x800) >> 11;
				if ((isControlCode && field) || isValidXDSCode) {
					check_channel += 2;
				}
				if (isControlCode && bchannel) {
					check_channel += 1;
				}
				if (cc_cnt > 1 && channel != check_channel) {
					output = false;
					log_write(LOG_TRACE, use_colors, "ReadRaw: Changing to channel %d from %d\n", check_channel, channel);
				}
				if (cc_cnt > 2) {
					output = false;
					log_write(LOG_TRACE, use_colors, "ReadRaw: Stopping output\n");
				}
				channel = check_channel;
			}
			if ((isControlCode && ((control_check == 0x2c) || (control_check == 0x2f))) || (isValidXDSCode && xds_check == 0xf)) {
				log_write(LOG_TRACE, use_colors, "ReadRaw: EOC, EDM, or XDS terminator recieved, setting eol\n");
				eol = true;
			}
			if (isControlCode && (control_check == 0x2d)) {
				received_cr = true;
			}
			else {
				received_cr = false;
			}
		}
		if (cc != 0 && !output) {
			log_write(LOG_TRACE, use_colors, "ReadRaw: Starting a new record for pts %d\n", (u32) current_frame);
			if (record_count != 0 && cc_cnt != 1) { // 1-based index; that condition should never happen
				((scc_entry*) output_ptr)->entry_count = cc_cnt-1;
				output_ptr += sizeof(scc_entry)+((cc_cnt-1)*sizeof(u16));
			}
			((scc_entry*) output_ptr)->pts.tc = int2tc(current_frame, fps, drop);
			record_count++;
			output = true;
			cc_cnt = 1;
		}
		log_write(LOG_TRACE, use_colors, "ReadRaw: CC data @ frame %08x: %04x (%c%c)\n", (u32) current_frame, cc, cc >> 8, cc & 0xff);
		((scc_entry*) output_ptr)->entries[cc_cnt-1] = cc;
		if ((((u32) end_ptr) - ((u32) output_ptr) - sizeof(u16)*cc_cnt) < 0x20) {
			scc_entry* _cc_data = realloc(cc_data, allocated + 8192);
			if (_cc_data == NULL) {
				log_write(LOG_FATAL, use_colors, "ReadRaw: Couldn't reallocate output buffer\n");
				free(cc_data);
				return NULL;
			}
			log_write(LOG_TRACE, use_colors, "ReadRaw: realloc success with %d bytes\n", allocated);
			// fix telemetry data
			output_ptr = (u8*) (((u32) _cc_data) + (((u32) output_ptr) - ((u32) cc_data)));
			cc_data = _cc_data;
			_cc_data = NULL;
			allocated+=8192;
			end_ptr = (u8*) cc_data+allocated;
			log_write(LOG_TRACE, use_colors, "ReadRaw: *** Pointer Locations ***\n\toutput_ptr 0x%08x\n\tend_ptr 0x%08x\n\tcc_data 0x%08x\n", (u32) output_ptr, (u32) end_ptr, (u32) cc_data);
		}
	}
	// We've reached the end. Set record count on the last CC entry.
	((scc_entry*) output_ptr)->entry_count = cc_cnt-1;
	output_ptr += sizeof(scc_entry)+((cc_cnt-1)*sizeof(u16));
	log_write(LOG_DEBUG, use_colors, "ReadRaw: Wrote %d bytes of CC data, from %d records of input\n", ((u32) output_ptr) - ((u32) cc_data), record_count);
	*length = ((u32) output_ptr) - ((u32) cc_data);
	return cc_data;
}
scc_entry* ReadNW4R(FILE* nw4r, size_t* length) {
	if (nw4r == NULL) {
		log_write(LOG_ERROR, use_colors, "ReadNW4R: Invalid file descriptor\n");
		return NULL;
	}
	bcc_hdr header;
	if (fread(&header, 1, 0x40, nw4r) != 0x40) {
NW4R_read_error:
		// check read error
		if (ferror(nw4r)) {
			log_write(LOG_ERROR, use_colors, "ReadNW4R: Error reading file (%d: %s)\n", errno, strerror(errno));
			return NULL;
		}
		// check eof
		else if (feof(nw4r)) {
			log_write(LOG_ERROR, use_colors, "ReadNW4R: unexpected end of file\n");
			return NULL;
		}
		else { // fread was successful but didn't return expected amount of bytes
			return NULL;
		}
	}
	if (memcmp(header.magic, "BCC1", 4) == 0 || memcmp(header.magic, "BCC2", 4) == 0) {
		bool8 swap = header.bom != 0xfeff;
		if (swap) {
			header.section_count = byteswap16(header.section_count);
			for (unsigned int i = 0; i < header.section_count; i++) {
				header.sections[i].offset = byteswap32(header.sections[i].offset);
				header.sections[i].size = byteswap32(header.sections[i].size);
			}
		}
		if (header.version_high != 1 && header.version_low != 0) {
			log_write(LOG_WARN, use_colors, "ReadNW4R: Header reports format version v%d.%d. File may not be compatible with this version of lib608.\n", header.version_high, header.version_low);
		}
		if (header.section_count == 0) {
			log_write(LOG_ERROR, use_colors, "ReadNW4R: Section count is 0!\n");
			return NULL;
		}
		for(int i = 0; i < header.section_count; i++) {
			if (header.sections[i].offset == 0) {
				continue;
			}
			ccdata_hdr data_hdr;
			fseek(nw4r, (s32) header.sections[i].offset, SEEK_SET);
			if (fread(&data_hdr, 1, 8, nw4r) != 8) {
				goto NW4R_read_error;
			}
			if (memcmp(data_hdr.magic, "DATA", 4) == 0) {
				u32 read_size = data_hdr.size;
				if (swap) {
					read_size = byteswap32(data_hdr.size);
				}
				if (header.sections[i].size != read_size) {
					log_write(LOG_WARN, use_colors, "ReadNW4R: size reported in DATA chunk and size reported in header do not match!\n");
					// take the lower size value
					read_size = read_size < header.sections[i].size ? read_size : header.sections[i].size;
				}
				read_size-=sizeof(ccdata_hdr);
				scc_entry* out = malloc(read_size);
				if (out == NULL) {
					log_write(LOG_FATAL, use_colors, "ReadNW4R: Couldn't allocate output buffer\n");
					return NULL;
				}
				fseek(nw4r, (s32) (header.sections[i].offset+sizeof(ccdata_hdr)), SEEK_SET);
				*length = fread(out, 1, read_size, nw4r);
				if (ferror(nw4r)) {
					log_write(LOG_ERROR, use_colors, "ReadNW4R: Error reading file (%d: %s)\n", errno, strerror(errno));
					// There may be CC data sucessfully read in before an error occurs; for example if the file gets deleted or rewritten midway through the read process, if an external USB/other device is unplugged, or some other I/O error occurs. This is why we do not bother to return NULL here, and since the data has already been malloc'd, it's safe to return the length reported by fread even if it's != 0. And if it is 0, the final output container will be conpletely empty with no additional data.
				}
				else if (feof(nw4r)) {
					log_write(LOG_WARN, use_colors, "ReadNW4R: unexpected end of file\n"); // Same message, different log level (here at least some CC data gets returned for sure)
				}
				else if (*length != read_size) { // else if, as "unexpected EOF" can cover this case, for example, if an weird I/O error occurs but fread doesn't return an error of any kind
					log_write(LOG_WARN, use_colors, "ReadNW4R: Expected %d, got %d (possible I/O error?)\n", read_size, *length);
				}
				// Either successful read, or an even weirder I/O error which can contain corrupted data
				if (swap) {
					unsigned int read_bytes = 0;
					u8* input_ptr = (u8*) out;
					scc_entry* entry = (scc_entry*) input_ptr;
					unsigned int entry_count = 0;
					while (read_bytes < *length) {
						entry_count = byteswap32(entry->entry_count);
						entry->entry_count = entry_count;
						entry->pts.raw= byteswap32(entry->pts.raw);
						for (unsigned int j=0; j < entry_count; j++) {
							entry->entries[j] = byteswap16(entry->entries[j]);
						}
						read_bytes += sizeof(scc_entry) + (sizeof(u16) * entry_count);
						input_ptr += sizeof(scc_entry) + (sizeof(u16) * entry_count);
						entry = (scc_entry*) input_ptr;
					}
				}
				log_write(LOG_DEBUG, use_colors, "ReadNW4R: Read 0x%08x bytes of input\n", *length);
				return out;
			}
			else {
				continue;
			}
		}
		log_write(LOG_ERROR, use_colors, "ReadNW4R: Input file is missing DATA section.\n");
		return NULL;
	}
	else {
		log_write(LOG_ERROR, use_colors, "ReadNW4R: Input is not a valid BCC NW4R file.\n");
		return NULL;
	}
}
u32 WriteRaw(scc_entry* in, size_t* length, FILE* out, f32 fps, timecode start, timecode end) {
	if (in == NULL) {
		log_write(LOG_FATAL, use_colors, "WriteRaw: invalid input pointer\n");
		return 0;
	}
	if (out == NULL) {
		log_write(LOG_ERROR, use_colors, "WriteRaw: invalid file descriptor\n");
		return 0;
	}
	unsigned int read_bytes = 0;
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
	unsigned int written_bytes = fwrite(file_header, 1, 4, out);
	if (ferror(out)) {
		goto raw_file_error;
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
				goto raw_file_error;
			}
		}
		current_frame+=(next_frame - current_frame);
		log_write(LOG_TRACE, use_colors, "WriteRaw: Current Frame %d\n", (s32) current_frame);
		for (unsigned int i = 0; i < entry->entry_count; i++) {
			log_write(LOG_TRACE, use_colors, "WriteRaw: CC to encode 0x%04x\n", entry->entries[i]);
			entry->entries[i] = fixParity(entry->entries[i]);
			u8 bytes[2] = {(u8) ((entry->entries[i] >> 8) & 0xff), (u8) (entry->entries[i] & 0xff)};
			log_write(LOG_TRACE, use_colors, "WriteRaw: Entry address 0x%08x\n", (u32) &(entry->entries[i]));
			u8* byte_pair = (u8*) &(entry->entries[i]);
			byte_pair[0] = bytes[0];
			byte_pair[1] = bytes[1];
			log_write(LOG_TRACE, use_colors, "WriteRaw: Encoded CC 0x%02x%02x\n", byte_pair[0], byte_pair[1]);
		}
		written_bytes += fwrite(entry->entries, 2, entry->entry_count, out);
		if (ferror(out)) {
			goto raw_file_error;
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
				goto raw_file_error;
			}
		}
	}
	else {
		// Write an extra 0x8080 at the end to match McPoodle's tools
		u16 write = fixParity(padding_bytes);
		written_bytes += fwrite(&write, 2, 1, out);
		if (ferror(out)) {
			goto raw_file_error;
		}
	}
	log_write(LOG_DEBUG, use_colors, "WriteRaw: wrote %d bytes, from %d bytes of input\n", written_bytes, *length);
	return written_bytes;
raw_file_error:
	log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
	return written_bytes;
}

u32 WriteNW4R(scc_entry* in, size_t* length, FILE* out, u8 field, bool8 swap) {
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
	header.sections[0].offset = sizeof(bcc_hdr);
	header.sections[0].size = sizeof(ccdata_hdr) + *length;
	if (swap) {
		header.bom = 0xfffe;
		header.size = byteswap32(header.size);
		header.header_size = byteswap16(header.header_size);
		for (int i = 0; i < header.section_count; i++) {
			header.sections[i].offset = byteswap32(header.sections[i].offset);
			header.sections[i].size = byteswap32(header.sections[i].size);
		}
	}
	header.section_count = byteswap16(header.section_count);
	unsigned int written_bytes = fwrite(&header, 1, sizeof(bcc_hdr), out);
	if (ferror(out)) {
		goto NW4R_file_error;
	}
	ccdata_hdr s1hdr = {0};
	memcpy(&s1hdr, &ccd_header, sizeof(ccdata_hdr));
	s1hdr.size = sizeof(ccdata_hdr) + *length;
	if (swap) {
		s1hdr.size = byteswap32(s1hdr.size);
	}
	written_bytes += fwrite(&s1hdr, 1, sizeof(ccdata_hdr), out);
	if (ferror(out)) {
		goto NW4R_file_error;
	}
	if (swap) {
		unsigned int read_bytes = 0;
		u8* input_ptr = (u8*) in;
		scc_entry* entry = (scc_entry*) input_ptr;
		unsigned int entry_count = 0;
		while (read_bytes < *length) {
			entry_count = entry->entry_count;
			entry->entry_count = byteswap32(entry_count);
			entry->pts.raw = byteswap32(entry->pts.raw);
			for (unsigned int i=0; i < entry_count; i++) {
				entry->entries[i] = byteswap16(entry->entries[i]);
			}
			read_bytes += sizeof(scc_entry) + (sizeof(u16) * entry_count);
			input_ptr += sizeof(scc_entry) + (sizeof(u16) * entry_count);
			entry = (scc_entry*) input_ptr;
		}
	}
	written_bytes += fwrite(in, 1, *length, out);
	if (ferror(out)) {
		goto NW4R_file_error;
	}
	log_write(LOG_DEBUG, use_colors, "WriteNW4R: wrote %d bytes, from %d bytes of input\n", written_bytes, *length);
	return written_bytes;
NW4R_file_error:
	log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
	return written_bytes;
}

bool8 IsRawFile(FILE* file) {
	bool8 ret = false;
	if (file == NULL) {
		log_write(LOG_ERROR, use_colors, "IsRawFile: Invalid file descriptor\n");
		return false;
	}
	u8 check[4];
	if (fread(&check, 1, 4, file) != 4) {
		// check read error
		if (ferror(file)) {
			log_write(LOG_ERROR, use_colors, "IsRawFile: Error reading file (%d: %s)\n", errno, strerror(errno));
			return false;
		}
		// check eof
		else if (feof(file)) {
			log_write(LOG_ERROR, use_colors, "IsRawFile: unexpected end of file\n");
			fseek(file, 0, SEEK_SET);
			return false;
		}
		else { // fread was successful but didn't return expected amount of bytes
			fseek(file, 0, SEEK_SET);
			return false;
		}
	}
	// Seek back to allow input functions and further checks to work properly
	fseek(file, 0, SEEK_SET);
	if (memcmp(check, file_header, 4) != 0) {
		ret = false;
	}
	else {
		ret = true;
	}
	log_write(LOG_DEBUG, use_colors, "IsRawFile: %s\n", ret ? "True" : "False");
	return ret;
}
bool8 IsNW4RFile(FILE* file) {
	bool8 ret = false;
	if (file == NULL) {
		log_write(LOG_ERROR, use_colors, "IsNW4RFile: Invalid file descriptor\n");
		return false;
	}
	bcc_hdr check;
	if (fread(&check, 1, 0x40, file) != 0x40) {
IsNW4R_read_error:
		// check read error
		if (ferror(file)) {
			log_write(LOG_ERROR, use_colors, "IsNW4RFile: Error reading file (%d: %s)\n", errno, strerror(errno));
			return false;
		}
		// check eof
		else if (feof(file)) {
			log_write(LOG_ERROR, use_colors, "IsNW4RFile: unexpected end of file\n");
			fseek(file, 0, SEEK_SET);
			return false;
		}
		else { // fread was successful but didn't return expected amount of bytes
			fseek(file, 0, SEEK_SET);
			return false;
		}
	}
	if (memcmp(check.magic, "BCC1", 4) == 0 || memcmp(check.magic, "BCC2", 4) == 0) {
		// Got ourselves an NW4R header, now check DATA header presence
		// NW4R format allows for the DATA section to not be the first section (at least it would, if there was more than one section)
		// With this loop, BRSTMs with its header's magic set to BCC1 instead of RSTM can be detected as our NW4R CC format... lol idgaf
		if (check.bom != 0xfeff) {
			check.section_count = byteswap16(check.section_count);
			for (unsigned int i = 0; i < check.section_count; i++) {
				check.sections[i].offset = byteswap32(check.sections[i].offset);
				check.sections[i].size = byteswap32(check.sections[i].size);
			}
		}
		if (check.section_count == 0) {
			ret = false;
			goto IsNW4R_end;
		}
		for(unsigned int i = 0; i < check.section_count; i++) {
			if (check.sections[i].offset == 0) {
				continue;
			}
			ccdata_hdr check2;
			fseek(file, (s32) check.sections[i].offset, SEEK_SET);
			if (fread(&check2, 1, 4, file) != 4) {
				goto IsNW4R_read_error;
			}
			if (memcmp(check2.magic, "DATA", 4) == 0) {
				ret = true;
				goto IsNW4R_end;
			}
			else {
				continue;
			}
		}
		ret = false;
	}
	else {
		ret = false;
	}
IsNW4R_end:
	// Seek back to allow input functions and further checks to work properly
	fseek(file, 0, SEEK_SET);
	log_write(LOG_DEBUG, use_colors, "IsNW4RFile: %s\n", ret ? "True" : "False");
	return ret;
}

u8 GetNW4RField(FILE* file) {
	u8 ret = 254; // Assume an even negative integer
	if (file == NULL) {
		log_write(LOG_ERROR, use_colors, "GetNW4RField: Invalid file descriptor\n");
		return 254;
	}
	bcc_hdr check;
	if (fread(&check, 1, 0x40, file) != 0x40) {
		// check read error
		if (ferror(file)) {
			log_write(LOG_ERROR, use_colors, "GetNW4RField: Error reading file (%d: %s)\n", errno, strerror(errno));
			return 254;
		}
		// check eof
		else if (feof(file)) {
			log_write(LOG_ERROR, use_colors, "GetNW4RField: unexpected end of file\n");
			fseek(file, 0, SEEK_SET);
			return 254;
		}
		else { // fread was successful but didn't return expected amount of bytes
			fseek(file, 0, SEEK_SET);
			return 254;
		}
	}
	if (memcmp(check.magic, "BCC1", 4) == 0) {
		ret = 0;
	}
	else if (memcmp(check.magic, "BCC2", 4) == 0) {
		ret = 1;
	}
	else {
		log_write(LOG_ERROR, use_colors, "GetNW4RField: Input is not a valid BCC NW4R file.\n");
		return 254;
	}
	fseek(file, 0, SEEK_SET);
	log_write(LOG_DEBUG, use_colors, "GetNW4RField: %hhu\n", ret);
	return ret;
}
