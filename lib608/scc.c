/*
scc.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "608.h"
#include "log.h"

scc_entry* ReadSCC(FILE* scc, size_t* length) {
	if (scc == NULL) {
		log_write(LOG_ERROR, use_colors, "ReadSCC: invalid file descriptor\n");
		return NULL;
	}
	u8 v1, v2;
	if (fscanf(scc, "Scenarist_SCC V%1hhd.%1hhd", &v1, &v2) != 2) {
		// check read error
		if (ferror(scc)) {
			log_write(LOG_ERROR, use_colors, "ReadSCC: Error reading file (%d: %s)\n", errno, strerror(errno));
			return NULL;
		}
		// check eof
		else if (feof(scc)) {
			log_write(LOG_ERROR, use_colors, "ReadSCC: unexpected end of file\n");
			return NULL;
		}
		log_write(LOG_ERROR, use_colors, "ReadSCC: Input is not an SCC file\n");
		return NULL;
	}
	if ((v1 != 1) || (v2 != 0)) {
		log_write(LOG_WARN, use_colors, "ReadSCC: SCC version not v1.0, decoding errors may happen\n");
	}
	log_write(LOG_DEBUG, use_colors, "Found Scenarist SCC file v%hhd.%hhd\n", v1, v2);
	char* read_buffer = malloc(4096); //overkill, but we gotta cover all the bases
	if (read_buffer == NULL) {
		log_write(LOG_FATAL, use_colors, "ReadSCC: couldn't allocate read buffer\n");
		return NULL;
	}
	int record_count = 0;
	int line = 1; //technically we're positioned before the newline at the end of the header so this will still work, lol
	timecode entry_tc = default_timecode;
	s16 hr = 0;
	u8 min = 0;
	u8 sec = 0;
	u8 frames = 0;
	bool8 df = false;
	char drop = ':';
	unsigned int allocated = 8192;
	scc_entry* cc_data = malloc(allocated);
	if (cc_data == NULL) {
		log_write(LOG_FATAL, use_colors, "ReadSCC: Memory allocation for output data failed\n");
		free(read_buffer);
		return NULL;
	}
	u8* output_ptr = (u8*) cc_data;
	u8* end_ptr = (u8*) cc_data+allocated;
	log_write(LOG_TRACE, use_colors, "ReadSCC: *** Pointer Locations ***\n\toutput_ptr 0x%08x\n\tend_ptr 0x%08x\n\tcc_data 0x%08x\n", (u32) output_ptr, (u32) end_ptr, (u32) cc_data);
	while (fgets(read_buffer, 4096, scc) != NULL) {
		if (sscanf(read_buffer, "%hd:%02hhd:%02hhd%c%02hhd", &hr, &min, &sec, &drop, &frames) != 5) {
			// Could've just been a newline, lol
			if ((strcmp("\n", read_buffer) == 0) | (strcmp("\r\n", read_buffer) == 0) | (strcmp("\n\r", read_buffer) == 0)) {
				line++;
				continue;
			}
			else {
				log_write(LOG_WARN, use_colors, "ReadSCC: Malformed timestamp at line %d (ignoring)\n", line);
				line++;
				continue;
			}
		}
		// assert consistent dropframe status
		if ((drop == ':') && (df)) {
			log_write(LOG_TRACE | LOG_LIBRARY, use_colors, "ReadSCC: inconsistent drop frame status, assuming non drop frame\n");
			df = false;
		}
		else if ((record_count != 0) && ((drop == ';') && (!df))) {
			log_write(LOG_TRACE | LOG_LIBRARY, use_colors, "ReadSCC: inconsistent drop frame status, assuming drop frame\n");
			df = true;
		}
		else if (drop == ';') {
			df = true;
		}
		else if (drop == ':') {
			df = false;
		}
		else {
			log_write(LOG_WARN, use_colors, "ReadSCC: Malformed timestamp at line %d (ignoring)\n", line);
			line++;
			continue;
		}
		// write the pts to the output buffer
		entry_tc.hours = (s16) hr;
		entry_tc.minutes = (u8) (min & 0x3f);
		entry_tc.seconds = (u8) (sec & 0x3f);
		entry_tc.frames = (u8) (frames & 0x7f);
		entry_tc.drop = (bool8) (df & 0x1);
		record_count++;
		char* cc_ptr = read_buffer+12;
		unsigned int caption_count = strlen(cc_ptr)/5;
		unsigned int decoded_cc_count = 0;
		u16 cc = 0;
		log_write(LOG_TRACE, use_colors, "ReadSCC: %d bytes away from end of allocated memory\n", ((u32) end_ptr) - ((u32) output_ptr));
		if (((u32) end_ptr) - ((u32) output_ptr) < 8+(caption_count*2)) {
			scc_entry* _cc_data = realloc(cc_data, allocated + 8192);
			if (_cc_data == NULL) {
				log_write(LOG_FATAL, use_colors, "ReadSCC: Couldn't reallocate output buffer\n");
				free(read_buffer);
				free(cc_data);
				return NULL;
			}
			log_write(LOG_TRACE, use_colors, "ReadSCC: realloc success with %d bytes\n", allocated);
			// fix telemetry data
			output_ptr = (u8*) (((u32) _cc_data) + (((u32) output_ptr) - ((u32) cc_data)));
			cc_data = _cc_data;
			_cc_data = NULL;
			allocated+=8192;
			end_ptr = (u8*) cc_data+allocated;
			log_write(LOG_TRACE, use_colors, "ReadSCC: *** Pointer Locations ***\n\toutput_ptr 0x%08x\n\tend_ptr 0x%08x\n\tcc_data 0x%08x\n", (u32) output_ptr, (u32) end_ptr, (u32) cc_data);
		}
		((scc_entry*) output_ptr)->pts.tc = entry_tc;
		for (unsigned int i = 0; i < caption_count; i++) {
			if (sscanf(cc_ptr+(i*5), "%04hx", &cc) != 1) {
				log_write(LOG_WARN, use_colors, "ReadSCC: Caption data at line %d is invalid\n", line);
				break;
			}
			((scc_entry*) output_ptr)->entries[i] = cc & 0x7f7f; // strip parity bits
			log_write(LOG_TRACE, use_colors, "ReadSCC: Decoded caption data: 0x%04hx\n", cc & 0x7f7f);
			decoded_cc_count++;
		}
		((scc_entry*) output_ptr)->entry_count = decoded_cc_count;
		output_ptr+=sizeof(scc_entry)+(decoded_cc_count*sizeof(u16));
		log_write(LOG_TRACE, use_colors, "ReadSCC: %d entries written for CC record %d (SCC line %d) @ %08x\n", decoded_cc_count, record_count, line, (u32) output_ptr);
		line++;
	}
	//free(read_buffer); // (apparently done by fgets above upon eof)
	log_write(LOG_DEBUG, use_colors, "ReadSCC: Wrote %d bytes of CC data, from %d lines of input\n", ((u32) output_ptr) - ((u32) cc_data), line);
	*length = ((u32) output_ptr) - ((u32) cc_data);
	return cc_data;
}

u32 WriteSCC(scc_entry* in, size_t* length, FILE* out) {
	if (in == NULL) {
		log_write(LOG_FATAL, use_colors, "WriteSCC: invalid input pointer\n");
		return 0;
	}
	if (out == NULL) {
		log_write(LOG_ERROR, use_colors, "WriteSCC: invalid file descriptor\n");
		return 0;
	}
	// Start with the SCC header...
	char* out_buf = (char*) malloc(4096); // is this overkill?
	if (out_buf == NULL) {
		log_write(LOG_FATAL, use_colors, "WriteSCC: Couldn't allocate output buffer\n");
		return 0;
	}
	sprintf(out_buf, "Scenarist_SCC V%1hhd.%1hhd\n", 1, 0); // TODO: Write the appropriate newline bytes for the host, instead of hardcoding Unix newlines (ReadSCC already accounts for this)
	unsigned int written_bytes = fwrite(out_buf, 1, strlen(out_buf), out);
	if (ferror(out)) {
		goto SCC_file_error;
	}
	unsigned int read_bytes = 0;
	u8* input_ptr = (u8*) in;
	scc_entry* entry = (scc_entry*) input_ptr;
	unsigned int entry_count = 0;
	while (read_bytes < *length) {
		// The timestamp
		timecode ts = entry->pts.tc;
		sprintf(out_buf, "\n%02d:%02hhd:%02hhd%c%02hhd\t", ts.hours, ts.minutes, ts.seconds, ts.drop ? ';' : ':', ts.frames); 
		written_bytes += fwrite(out_buf, 1, strlen(out_buf), out);
		if (ferror(out)) {
			goto SCC_file_error;
		}
		// The entries
		entry_count = entry->entry_count;
		log_write(LOG_DEBUG, use_colors, "WriteSCC: processing %d records for pts 0x%08x\n", entry_count, entry->pts.raw);
		for (unsigned int i=0; i < entry_count; i++) {
			sprintf(out_buf, "%04hx%c", fixParity(entry->entries[i]), (i + 1) == entry_count ? '\n' : ' ');
			written_bytes += fwrite(out_buf, 1, strlen(out_buf), out);
			if (ferror(out)) {
				goto SCC_file_error;
			}
		}
		read_bytes += sizeof(scc_entry) + (sizeof(u16) * entry_count);
		input_ptr += sizeof(scc_entry) + (sizeof(u16) * entry_count);
		entry = (scc_entry*) input_ptr;
	}
	log_write(LOG_DEBUG, use_colors, "WriteSCC: Wrote %d bytes, from %d bytes of input\n", written_bytes, *length);
	free(out_buf);
	return written_bytes;
SCC_file_error:
	log_write(LOG_ERROR, use_colors, "Error writing file (%d: %s)\n", errno, strerror(errno));
	return written_bytes;
}

bool8 IsSCCFile(FILE* file) {
	bool8 ret = false;
	if (file == NULL) {
		log_write(LOG_ERROR, use_colors, "IsSCCFile: invalid file descriptor\n");
		return false;
	}
	u8 v1, v2;
	if (fscanf(file, "Scenarist_SCC V%1hhd.%1hhd", &v1, &v2) != 2) { // Account for different version file. ReadSCC already warns if this isn't 1.0, so no need to do that here
		// check read error
		if (ferror(file)) {
			log_write(LOG_ERROR, use_colors, "IsSCCFile: Error reading file (%d: %s)\n", errno, strerror(errno));
			return false;
		}
		// check eof
		else if (feof(file)) {
			log_write(LOG_ERROR, use_colors, "IsSCCFile: unexpected end of file\n");
			fseek(file, 0, SEEK_SET);
			return false;
		}
		ret = false; // if file was read successfully but is not SCC format
	}
	else {
		ret = true;
	}
	// Seek back to allow input functions and further checks to work properly
	fseek(file, 0, SEEK_SET);
	log_write(LOG_DEBUG, use_colors, "IsSCCFile: %s\n", ret ? "True" : "False");
	return ret;
}
