/*
scc2raw.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <getopt.h>

#include "608.h"
#include "version.h"

const VersionInfo versionInfo = {
	0,
	0,
	0,
	1,
	""
}; // todo: proper git integration

int main(int argc, char **argv) {
	bool8 dvdmode = false;
	f32 fps = 30/1.001f;
	int c;

	struct option long_options[] = {
		{"input", required_argument, 0, "i"},
		{"fps", required_argument, 0, 0x80},
		{"field1", no_argument, 0, "1"},
		{"field2", no_argument, 0, "2"},
		{"mode", required_argument, 0, "m"},
		{"input2", required_argument, 0, 0x81},
		{"verbose", no_argument, 0, "v"},
		{"version", no_argument, 0, 0x82},
		{"help", no_argument, 0, "h"},
		{0, 0, 0, 0}
	}
}