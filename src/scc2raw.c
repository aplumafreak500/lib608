/*
scc2raw.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdlib.h>
#include <getopt.h>

#include "608.h"
#include "version.h"
#include "log.h"

static const VersionInfo versionInfo = {0, 0, 0, 1, ""}; // todo: proper git integration

int main(int argc, char **argv) {
	// prog_header();
	change_log_level(0xff);
	bool8 dvdmode = false;
	f32 fps = 30.0f;
	bool8 field1 = false;
	bool8 field2 = false;
	bool8 dropframe = true;
	char* file_path = NULL;
	char* file_path2 = NULL;

	// todo: check for color support (pipes vs console)
	bool8 use_colors = true;

	int c;

	struct option long_options[] = {
		{"input", required_argument, 0, 'i'},
		{"fps", required_argument, 0, 0x80},
		{"field1", no_argument, 0, '1'},
		{"field2", no_argument, 0, '2'},
		{"mode", required_argument, 0, 'm'},
		{"input2", required_argument, 0, 0x81},
		{"verbose", no_argument, 0, 'v'},
		{"quiet", no_argument, 0, 'q'},
		{"version", no_argument, 0, 0x82},
		{"log_level", required_argument, 0, 0x83},
		{"help", no_argument, 0, 'h'},
		{"dropframe", no_argument, 0, 'd'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	opterr = 0;

	while(1) {
		int curind = optind;
		c = getopt_long(argc, argv, ":12dhi:m:qv", long_options, &option_index);
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
				// change_log_level(LOG_FATAL | LOG_ERROR);
				break;
			case 'h':
				// usage();
				exit(0);
				return 1;
			case 'i':
				log_write(LOG_DEBUG, use_colors, "in = %s\n", optarg);
				file_path = optarg;
				break;
			case 'm':
				log_write(LOG_DEBUG, use_colors, "mode = %s\n", optarg);
				// todo: dvd/raw
				break;
			case 'v':
				// change_log_level(LOG_VERBOSE);
				break;
			case 0x80:
				log_write(LOG_DEBUG, use_colors, "fps = %s\n", optarg);
				// todo: char* to f32
				break;
			case 0x81:
				file_path2 = optarg;
				break;
			case 0x82:
				// version();
				exit(0);
				return 2;
			case 0x83:
				log_write(LOG_DEBUG, use_colors, "log level = %s\n", optarg);
				// todo: char* to u8
				break;
			case ':':
				if (optopt < 0x80) {
					log_write(LOG_ERROR, use_colors, "Option -%c requires an argument\n", optopt);
				}
				else {
					log_write(LOG_ERROR, use_colors, "Option %s requires an argument\n", argv[curind]);
				}
				exit(1);
				return 4;
			case '?':
  	      default:
				if (optopt | (optopt < 0x80)) {
					log_write(LOG_ERROR, use_colors, "Invalid option -%c\n", optopt);
				}
				else {
					log_write(LOG_ERROR, use_colors, "Invalid option %s\n", argv[curind]);
				}
				exit(1);
				return 3;
		}
	}
	// todo: output file
	if (optind < argc) {
		log_write(LOG_DEBUG, use_colors, "non-option ARGV-elements: ");
		while (optind < argc) {
			log_write(LOG_DEBUG, use_colors, "%s ", argv[optind++]);
		}
		log_write(LOG_DEBUG, use_colors, "\n");
	}
	// todo: call library function
	exit(0);
	return 0;
}