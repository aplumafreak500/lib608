/*
raw2scc.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "608.h"
#include "config.h" // for git
#include "log.h"

static const VersionInfo versionInfo = {0, 5, 0, 0, ""}; // todo: proper git integration

enum{
	MODE_RAW,
	MODE_DVD,
	MODE_NW4R
}; // This is used to select the input format

static void prog_header(char* name);
static void usage(char* name);

int main(int argc, char **argv) {
	u8 log_level = LOG_DEFAULT;
	// todo: check for color support (pipes vs console)
	use_colors = true;
	prog_header(argv[0]);
	change_log_level(log_level);
	u8 mode = MODE_RAW;
	f32 fps = 30/1.001f;
	bool8 field1 = false;
	bool8 field2 = false;
	bool8 drop = false;
	char* file_path = NULL;
	char* output_file = NULL;
	char* output_file2 = NULL;
	timecode start_timecode = default_timecode;
	s16 stc_hrs;
	u8 stc_min;
	u8 stc_sec;
	u8 stc_frames;
	int c;

	const struct option long_options[] = {
		{"input", required_argument, 0, 'i'},
		{"fps", required_argument, 0, 0x80},
		{"field1", no_argument, 0, '1'},
		{"field2", no_argument, 0, '2'},
		{"verbose", no_argument, 0, 'v'},
		{"quiet", no_argument, 0, 'q'},
		//{"version", no_argument, 0, 0x82},
		{"start_time", required_argument, 0, 0x84},
		{"limit", required_argument, 0, 'l'},
		{"log_level", required_argument, 0, 0x83},
		{"help", no_argument, 0, 'h'},
		{"dropframe", no_argument, 0, 'd'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	opterr = 0;

	while(1) {
		int curind = optind;
		c = getopt_long(argc, argv, ":12dhi:l:qv", long_options, &option_index);
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
			case 'd':
				drop = true;
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
		case 'l':
				if (sscanf(optarg, "%d", &MAX_NULLS) == 0) {
					log_write(LOG_WARN, use_colors, "Invalid parameter for option --limit: %s (will assume 2)\n", optarg);
					MAX_NULLS = 2;
				}
				log_write(LOG_DEBUG, use_colors, "8080 limit = %d\n", MAX_NULLS);
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
			case 0x82:
				// version();
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
				start_timecode.hours = stc_hrs;
				start_timecode.minutes = (u8) (stc_min & 0x3f);
				start_timecode.seconds = (u8) (stc_sec & 0x3f);
				start_timecode.frames = (u8) (stc_frames & 0x7f);
				log_write(LOG_DEBUG, use_colors, "start_tc %02hd:%02hhu:%02hhu:%02hhu\n", start_timecode.hours, start_timecode.minutes, start_timecode.seconds, start_timecode.frames);
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
	/*if (optind < argc) {
		output_file2 = argv[optind++];
		log_write(LOG_DEBUG, use_colors, "second output to %s...\n", output_file);
	}*/
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

	const char* mode_str[] = {"raw", "dvd", "nw4r"};

	if (IsNW4RFile(in_file)) {
		mode = MODE_NW4R;
	}
	else if (IsRawFile(in_file)) {
		mode = MODE_RAW;
	}
	// else if (IsDVDFile(in_file)) {
		// mode = MODE_DVD;
	// }
	else {
		log_write(LOG_ERROR, use_colors, "Input is not in a recognized format!\n");
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

	// DVD format second file
	FILE* out_file2 = NULL;

	if (mode == MODE_DVD) {
		if ((output_file2 == NULL) || (strcmp("", output_file2) == 0)) {
			if (field1 && field2) {
				log_write(LOG_WARN, use_colors, "Fields 1 and 2 are set, but second output is unspecified. Field 1 will only be output.\n");
				field1 = true;
				field2 = false;
			}
		}
		out_file2 = fopen(output_file2, "r");
		if (out_file2 == NULL) {
			if (field1 && field2) {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), Field 1 will only be output.\n", output_file2, errno, strerror(errno));
				field1 = true;
				field2 = false;
			}
			else if (!field1 && field2) {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), Field 2 will only be output.\n", output_file2, errno, strerror(errno));
				field1 = false;
				field2 = true;
			}
			else {
				log_write(LOG_WARN, use_colors, "Can't open file %s (%d: %s), Field 1 will only be output.\n", output_file2, errno, strerror(errno));
				field1 = true;
				field2 = false;
			}
			output_file2 = NULL;
		}
	}
	else if (mode == MODE_NW4R) {
		u8 field = GetNW4RField(in_file);
		if (field == 0) {
			field1 = true;
			field2 = false;
		}
		else if (field == 1) {
			field1 = false;
			field2 = true;
		}
		else { // Should never happen, as we've already called IsNW4RFile
			fclose(in_file);
			fclose(out_file);
			if (out_file2 != NULL) {
				fclose(out_file2);
			}
			return 6;
		}
	}
	else if (mode == MODE_RAW) {
		if (field1 && field2) {
			log_write(LOG_WARN, use_colors, "Detected raw format, which only supports one field, and both fields are specified. Assuming Field 1.\n");
			field1 = true;
			field2 = false;
		}
		if (!(field1 || field2)) {
			field1 = true;
			field2 = false;
		}
	}
	else { // Should never happen
		log_write(LOG_FATAL, use_colors, "Invalid mode set (possibly corrupted memory?)\n");
		fclose(in_file);
		fclose(out_file);
		if (out_file2 != NULL) {
			fclose(out_file2);
		}
		return -1;
	}

	log_write(LOG_INFO, use_colors, "Input: %s\nOutput: %s\nInput Format: %s\nFPS: %f\nTimestamp Offset: %02d:%02hhu:%02hhu:%02hhu\nFields: %s%s", file_path, output_file, mode_str[mode], fps, start_timecode.hours, start_timecode.minutes, start_timecode.seconds, start_timecode.frames, field1 ? "1" : "", field2 ? "2" : "");

	// file 2
	if (output_file2 != NULL) {
		log_write(LOG_INFO, false, "Output 2: %s", output_file2);
	}
	log_write(LOG_INFO, false, "\n");
	
	unsigned int read_ccs;
	scc_entry* ccd;
	//unsigned int read_ccs2;
	//scc_entry* ccd2;
	if (mode == MODE_RAW) ccd=ReadRaw(in_file, &read_ccs, fps, start_timecode, drop);
	else if (mode == MODE_NW4R) ccd=ReadNW4R(in_file, &read_ccs);
	// else if (mode == MODE_DVD) ReadDVD(in_file, ccd, &read_ccs, ccd2, &read_ccs2, fps, start_timecode);
	log_write(LOG_TRACE, use_colors, "address of ccd 0x%08x\n", (u32) ccd);
	if (ccd == NULL) {
		fclose(in_file);
		fclose(out_file);
		if (out_file2 != NULL) {
			fclose(out_file2);
		}
		return 5;
	}

	//if (mode == MODE_DVD && ccd2 == NULL) {
		//log_write(LOG_WARN, use_colors, "Field 2 caption data not returned, Field 1 will only be output.\n");
		//if (out_file2 != NULL) {
			//fclose(out_file2);
		//}
	//}

	WriteSCC(ccd, &read_ccs, out_file);
	// comment out to prevent "maybe used uninitialized" warning
	//if (mode == MODE_DVD && ccd2 != NULL) WriteSCC(ccd2, &read_ccs2, out_file2);

	if (ccd != NULL) {
		free(ccd);
	}
	//if (ccd2 != NULL) {
		//free(ccd2);
	//}
	fclose(in_file);
	fclose(out_file);
	if (out_file2 != NULL) {
		fclose(out_file2);
	}
	return 0;
}

static void prog_header(char* name) {
	log_write(LOG_APPLICATION, false, "%s version %hd.%hd.%hd.%hd", name, versionInfo.major, versionInfo.minor, versionInfo.revision, versionInfo.build);
	if (strcmp("", versionInfo.git_rev) != 0) {
		log_write(LOG_APPLICATION, false, " g%s", versionInfo.git_rev);
	}
	log_write(LOG_APPLICATION, false, "\nlib608 version: %hd.%hd.%hd.%hd", library_version.major, library_version.minor, library_version.revision, library_version.build);
	if (strcmp("", library_version.git_rev) != 0) {
		log_write(LOG_APPLICATION, false, " g%s", library_version.git_rev);
	}
	log_write(LOG_APPLICATION, false, "\n\n%s is distributed under the terms of the GNU General Public License v3 or later; view these terms in the included License.txt file.\n\n", name);
}

static void usage(char* name) {
	log_write(LOG_APPLICATION, false,
	"The basic usage is:\n"
	"\n%s -i <input> <output>\n"
	/*"If the input is DVD format, a second output can be specified.\n*/"\n"
	"Detailed option listing:\n"
	"--input\t-i <file>\n"
	"\tSpecifies in input file (required)\n"
	"--fps <fps>\n"
	"\tSpecifies fps (For raw and dvd output)\n"
	"--field[1|2]\t-[1|2]\n"
	/*"\tFor DVD input, specify which fields to output.\n"*/
	//"\tFor NW4R input, this is autodetected.\n"
	"--verbose\t-v\n"
	"\tBe more verbose.\n"
	"--quiet\t-q\n"
	"\tOnly output errors.\n"
	"--start-time <00:00:00:00>\n"
	"\tSpecifies an offset to be applied to the input material.\n"
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
	"--dropframe\t-d\n"
	"\tSpecifies dropframe for the input\n"
	"--help\t-h\n"
	"\tShows this info\n"
	/*"--version\n"
	"\tVersion info\n"*/
	"\n\n", name);
}
