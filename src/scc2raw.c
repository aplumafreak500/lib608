/*
scc2raw.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "608.h"
#include "config.h" // for git
#include "log.h"

static const VersionInfo versionInfo = {0, 5, 0, 1, VERSION}; // todo: proper git integration

enum{
	MODE_RAW,
	MODE_DVD,
	MODE_NW4R
};

static void prog_header(char* name);
static void usage(char* name);

int main(int argc, char **argv) {
	u8 log_level = LOG_DEFAULT;
	// is this the right way to do that?
	use_colors = isatty(STDERR_FILENO);
	prog_header(argv[0]);
	change_log_level(log_level);
	u8 mode = MODE_RAW;
	f32 fps = 29.97003f; // HACK: just above 29.97 fps to prevent false positives for out of order scc input 
	bool8 field1 = false;
	bool8 field2 = false;
	bool8 swap = !WORDS_BIGENDIAN;
	char* file_path = NULL;
	char* file_path2 = NULL;
	char* output_file = NULL;
	timecode start_timecode = default_timecode;
	s16 stc_hrs;
	u8 stc_min;
	u8 stc_sec;
	u8 stc_frames;
	timecode pad_tc = default_timecode;
	int c;

	const struct option long_options[] = {
		{"input", required_argument, 0, 'i'},
		{"fps", required_argument, 0, 0x80},
		{"field1", no_argument, 0, '1'},
		{"field2", no_argument, 0, '2'},
		{"swap", no_argument, 0, 0x86},
		{"mode", required_argument, 0, 'm'},
		//{"input2", required_argument, 0, 0x81},
		{"verbose", no_argument, 0, 'v'},
		{"quiet", no_argument, 0, 'q'},
		//{"version", no_argument, 0, 0x82},
		//{"start_time", required_argument, 0, 0x84},
		//{"end_time", required_argument, 0, 0x85},
		{"log_level", required_argument, 0, 0x83},
		{"help", no_argument, 0, 'h'},
		//{"dropframe", no_argument, 0, 'd'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	opterr = 0;

	while(1) {
		int curind = optind;
		c = getopt_long(argc, argv, ":12"/*d*/"hi:m:qv", long_options, &option_index);
		if (c == -1) {
			log_write(LOG_TRACE, use_colors, "Finished parsing command line options.\n");
			break;
		}
		switch (c) {
			case 0:
				log_write(LOG_WARN, use_colors, "getopt_long Type 2 option, shouldn't happen normally\n");
				break;
			case '1':
				field1 = true;
				break;
			case '2':
				field2 = true;
				break;
			case 'q':
				change_log_level(LOG_FATAL | LOG_ERROR);
				break;
			case 'h':
				usage(argv[0]);
				return 2;
			case 'i':
				log_write(LOG_DEBUG, use_colors, "in = %s\n", optarg);
				file_path = optarg;
				break;
			case 'm':
				/*if (strcasecmp("dvd", optarg) == 0) {
					mode = MODE_DVD;
				}
				else*/ if (strcasecmp("raw", optarg) == 0) {
					mode = MODE_RAW;
				}
				else if (strcasecmp("nw4r", optarg) == 0) {
					mode = MODE_NW4R;
				}
				else {
					log_write(LOG_WARN, use_colors, "--mode must be either "/*'dvd', */"'raw' or 'nw4r', assuming raw mode\n", optarg);
					mode = MODE_RAW;
				}
				log_write(LOG_DEBUG, use_colors, "dvdmode = %hhu\n", mode);
				break;
			case 'v':
				change_log_level(LOG_VERBOSE);
				break;
			case 0x80:
				if (sscanf(optarg, "%f", &fps) == 0) {
					log_write(LOG_WARN, use_colors, "Invalid parameter for option --fps: %s (will assume 29.97 fps)\n", optarg);
					fps = 30.0f/1.001f;
				}
				log_write(LOG_DEBUG, use_colors, "fps = %f\n", fps);
				break;
			case 0x81:
				file_path2 = optarg;
				break;
			case 0x82:
				// version(); TODO
				return 2;
			case 0x83:
				if (sscanf(optarg, "%hhu", &log_level) == 0) {
					log_write(LOG_WARN, use_colors, "Invalid parameter for option --log_level: %s (assuming %d)\n", optarg, LOG_DEFAULT);
				}
				log_write(LOG_DEBUG, use_colors, "log level = %hhu\n",  log_level);
				change_log_level(log_level);
				break;
			case 0x84:
				sscanf(optarg, "%02hd:%02hhu:%02hhu:%02hhu", &stc_hrs, &stc_min, &stc_sec, &stc_frames);
				start_timecode.hours = (u16) stc_hrs;
				start_timecode.minutes = (u8) (stc_min & 0x3f);
				start_timecode.seconds = (u8) (stc_sec & 0x3f);
				start_timecode.frames = (u8) (stc_frames & 0x7f);
				log_write(LOG_DEBUG, use_colors, "start_tc %02hd:%02hhu:%02hhu:%02hhu\n", start_timecode.hours, start_timecode.minutes, start_timecode.seconds, start_timecode.frames);
				break;
			case 0x85:
				sscanf(optarg, "%02hd:%02hhu:%02hhu:%02hhu", &stc_hrs, &stc_min, &stc_sec, &stc_frames);
				pad_tc.hours = (u16) stc_hrs;
				pad_tc.minutes = (u8) (stc_min & 0x3f);
				pad_tc.seconds = (u8) (stc_sec & 0x3f);
				pad_tc.frames = (u8) (stc_frames & 0x7f);
				log_write(LOG_DEBUG, use_colors, "end_tc %02hd:%02hhu:%02hhu:%02hhu\n", pad_tc.hours, pad_tc.minutes, pad_tc.seconds, pad_tc.frames);
				break;
			case 0x86:
				swap = !swap;
				break;
			case ':':
				if (optopt < 0x80) {
					log_write(LOG_ERROR, use_colors, "Option -%c requires an argument\n", optopt);
				}
				else {
					log_write(LOG_ERROR, use_colors, "Option %s requires an argument\n", argv[curind]);
				}
				return 1;
			case '?':
  	      default:
				if (optopt) {
					log_write(LOG_WARN, use_colors, "Invalid option -%c (ignoring)\n", optopt);
				}
				else {
					log_write(LOG_WARN, use_colors, "Invalid option %s (ignoring)\n", argv[curind]);
				}
				break;
		}
	}
	if (optind < argc) {
		output_file = argv[optind++];
		log_write(LOG_DEBUG, use_colors, "output to %s...\n", output_file);
	}
	if (optind < argc) {
		while (optind < argc) {
			log_write(LOG_WARN, use_colors, "Trailing option %s was found (ignoring).\n", argv[optind++]);
		}
	}

	if ((file_path == NULL) || (strcmp("", file_path) == 0)) {
		log_write(LOG_ERROR, use_colors, "An input file is required.\n");
		return 3;
	}

	FILE* in_file = fopen(file_path, "r");
	if (in_file == NULL) {
		log_write(LOG_ERROR, use_colors, "Can't open file %s (%d: %s)\n", file_path, errno, strerror(errno));
		return 3;
	}

	if (!IsSCCFile(in_file)) {
		log_write(LOG_ERROR, use_colors, "Input is not an SCC file!\n");
		fclose(in_file);
		return 6;
	}

	if ((output_file == NULL) || (strcmp("", output_file) == 0)) {
		log_write(LOG_ERROR, use_colors, "An output file is required.\n");
		fclose(in_file);
		return 3;
	}

	FILE* out_file = fopen(output_file, "w+");
	if (out_file == NULL) {
		log_write(LOG_ERROR, use_colors, "Can't open file %s (%d: %s)\n", output_file, errno, strerror(errno));
		fclose(in_file);
		return 3;
	}

	const char* mode_str[] = {"raw", "dvd", "nw4r"};

	if (field1 && field2 && mode != MODE_DVD) {
		log_write(LOG_ERROR, use_colors, "Mode %s can't contain both fields 1 and 2!\n", mode_str[mode]);
		return 4;
	}

	if (mode != MODE_DVD && file_path2 != NULL) {
		log_write(LOG_WARN, use_colors, "Mode != dvd, not opening file %s\n", file_path2);
		file_path2 = NULL;
	}

	// DVD format second file
	FILE* in_file2 = NULL;

	// Raw format only supports Field 1. If fields are unset or if only Field 2 is set, set Field 1 and unset Field 2.
	if (mode == MODE_RAW) {
		field1 = true;
		field2 = false;
	}
	// NW4R format supports Field 1 or 2, but not both at once. If none was specified, we assume Field 1.
	else if (mode == MODE_NW4R) {
		if (!field1 && !field2) {
			field1=true;
		}
	}
	// DVD format can accept two inputs.
	else if (mode == MODE_DVD) {
		if ((file_path2 == NULL) || (strcmp("", file_path2) == 0)) {
			if (field1 && field2) {
				log_write(LOG_WARN, use_colors, "Fields 1 and 2 are set, but second input is unspecified. Output will only contain Field 1.\n");
				field1 = true;
				field2 = false;
			}
		}
		in_file2 = fopen(file_path2, "r");
		if (in_file2 == NULL) {
			if (field1 && field2) {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), output will only contain Field 1.\n", file_path2, errno, strerror(errno));
				field1 = true;
				field2 = false;
			}
			else if (!field1 && field2) {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), output will only contain Field 2.\n", file_path2, errno, strerror(errno));
				field1 = false;
				field2 = true;
			}
			else {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), output will only contain Field 1.\n", file_path2, errno, strerror(errno));
				field1 = true;
				field2 = false;
			}
			file_path2 = NULL;
		}
	}
	else { // Should never happen
		log_write(LOG_FATAL, use_colors, "Invalid mode set (possibly corrupted memory?)\n");
		fclose(in_file);
		if (in_file2 != NULL) {
			fclose(in_file2);
		}
		fclose(out_file);
		return -1;
	}

	// For NW4R format
	u8 field = 254; // for extreme edge cases, use an even negative number so Field 1 can be assumed
	if (field1 && !field2) field = 0;
	else if (!field1 && field2) field = 1;

	if (mode != MODE_NW4R && swap != !WORDS_BIGENDIAN) {
		log_write(LOG_WARN, use_colors, "Option --swap is relevant only when mode = NW4R (ignoring) \n");
		swap = !WORDS_BIGENDIAN;
	}

	log_write(LOG_INFO, use_colors, "Input: %s\nOutput: %s\nFPS: %f\nMode: %s\n"/*Timestamp Offset: %02hd:%02hhu:%02hhu:%02hhu\n*/"Fields: %s%s", file_path, output_file, fps, mode_str[mode], /*start_timecode.hours, start_timecode.minutes, start_timecode.seconds, start_timecode.frames,*/ field1 ? "1" : "", field2 ? "2" : "");

	// file 2
	if (file_path2 != NULL) {
		log_write(LOG_INFO, false, "Input 2: %s", file_path2);
	}
	log_write(LOG_INFO, false, "\n");
	if (mode == MODE_NW4R) {
		log_write(LOG_INFO, false, "NW4R Endianness: %s", swap ? "big": "little");
	}
	log_write(LOG_INFO, false, "\n");

	u32 read_ccs;
	scc_entry* ccd = ReadSCC(in_file, &read_ccs);
	log_write(LOG_TRACE, use_colors, "address of ccd 0x%08x\n", (u32) ccd);
	if (ccd == NULL) {
		// error reporting done within function
		fclose(in_file);
		if (in_file2 != NULL) {
			fclose(in_file2);
		}
		fclose(out_file);
		return 5;
	}

	// DVD format
	u32 read_ccs2 = 0;
	scc_entry* ccd2 = NULL;
	if (mode == MODE_DVD && in_file2 != NULL) { 
		ccd2 = ReadSCC(in_file2, &read_ccs2);
		log_write(LOG_TRACE, use_colors, "address of ccd2 0x%08x\n", (int) ccd2);
		if (ccd2 == NULL) {
			fclose(in_file);
			fclose(in_file2);
			fclose(out_file);
			return 5;
		}
	}

	if (mode == MODE_RAW) {
		WriteRaw(ccd, &read_ccs, out_file, fps, start_timecode, pad_tc);
	}
	else if (mode == MODE_DVD) {
		if (!field1 && field2) {
			//WriteDVD(ccd2, &read_ccs2, ccd, &read_ccs, out_file, fps, start_timecode, pad_tc, 5);
		}
		else {
			//WriteDVD(ccd, &read_ccs, ccd2, &read_ccs2, out_file, fps, start_timecode, pad_tc, 5);
		}
	}
	else {
		WriteNW4R(ccd, &read_ccs, out_file, field, swap);
	}

	if (ccd != NULL) {
		free(ccd);
	}
	if (ccd2 != NULL) {
		free(ccd2);
	}
	fclose(in_file);
	if (in_file2 != NULL) {
		fclose(in_file2);
	}
	fclose(out_file);
	return 0;
}

static void prog_header(char* name) {
	log_write(LOG_APPLICATION, false, "%s version %hd.%hd.%hd.%hd", name, versionInfo.major, versionInfo.minor, versionInfo.revision, versionInfo.build);
	if (strcmp("", versionInfo.git_rev) != 0) {
		log_write(LOG_APPLICATION, false, " %s", versionInfo.git_rev);
	}
	log_write(LOG_APPLICATION, false, "\nlib608 version: %hd.%hd.%hd.%hd", library_version.major, library_version.minor, library_version.revision, library_version.build);
	if (strcmp("", library_version.git_rev) != 0) {
		log_write(LOG_APPLICATION, false, " %s", library_version.git_rev);
	}
	log_write(LOG_APPLICATION, false, "\n\n%s is distributed under the terms of the GNU General Public License v3 or later; view these terms in the included License.txt file.\n\n", name);
}

static* void usage(char* name) {
	log_write(LOG_APPLICATION, false,
	"The basic usage is:\n"
	"\n%s -i <input> <output>\n\n"
	"Detailed option listing:\n"
	"--input\t-i <file>\n"
	"\tSpecifies in input file (required)\n"
	"--fps <fps>\n"
	"\tSpecifies fps (For raw and dvd output)\n"
	"--field[1|2]\t-[1|2]\n"
	/*"\tFor DVD output, specify which fields to include.\n"*/
	"\tFor NW4R output, controls the \"field\" value in the file's header.\n"
	"--swap\n"
	"\tFor NW4R output, output little-endian files.\n"
	"--mode\t-m [raw|nw4r]\n"/*|dvd]\n"*/
	"\tSpecify output format.\n"
	/*"--input2 <file>\n"
	"\tSpecifies a second input file for DVD output.\n"*/
	"--verbose\t-v\n"
	"\tBe more verbose.\n"
	"--quiet\t-q\n"
	"\tOnly output errors.\n"
	/*"--start-time <00:00:00:00>\n"
	"\tSpecifies an offset to be applied to the input material.\n"
	"--end-time <00:00:00:00>\n"
	"\tFor DVD or raw output, pad the file with 8080 until this timecode.\n"*/
	"--log_level <level>\n"
	"\tSpecify a custom log level. Defaults to 207. Log bitmasks are as follows:\n"
	"\t\t1: Fatal\n"
	"\t\t2: Error\n"
	"\t\t4: Warning\n"
	"\t\t8: Info\n"
	"\t\t16: Debug\n"
	"\t\t32: Trace\n"
	"\t\t64: Library messages\n"
	"\t\t128: Application messages (internally only)\n"
	/*"--dropframe\t-d\n"
	"\tSpecifies dropframe for the input\n"*/
	"--help\t-h\n"
	"\tShows this info\n"
	/*"--version\n"
	"\tVersion info\n"*/
	"\n\n", name);
}
