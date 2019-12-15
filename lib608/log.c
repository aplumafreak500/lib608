/*
log.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <stdio.h>
#include <stdarg.h>
#include "608.h"
#include "log.h"

static u8 log_level = LOG_DEFAULT;
bool8 use_colors = false;

static const struct {
	u8 level;
	char color_code[7];
} colors[9] = {
	{LOG_FATAL, "\x1b[31;m"},
	{LOG_ERROR, "\x1b[91;m"},
	{LOG_WARN, "\x1b[93;m"},
	{LOG_INFO, "\x1b[92;m"},
	{LOG_DEBUG, "\x1b[94;m"},
	{LOG_TRACE, "\x1b[32;m"},
	{LOG_LIBRARY, "\x1b[35;m"},
	{LOG_APPLICATION, "\x1b[36;m"},
	{0, "\x1b[m;"},
};

int log_write(u8 level, bool8 color, char *fmt, ...) {
	if ((log_level & level) == 0) {
		return 0;
	}
	int result = 0;
	if (color) {
		const char* colorcode = NULL;
		for (int i = 0; i < 9; i++) {
			if ((colors[i].level & level) != 0) {
				colorcode = colors[i].color_code;
				break;
			}
			if (colors[i].level == 0) {
				colorcode = "";
				break;
			}
		}
		result = fprintf(stderr, "%s", colorcode);
	}
	va_list args;
	va_start(args, fmt);
	result += vfprintf(stderr, fmt, args);
	va_end(args);
	return result;
}

u8 change_log_level(u8 newLevel) {
	log_level = newLevel;
	return log_level;
}

u8 reset_log_level() {
	log_level = LOG_DEFAULT;
	return log_level;
}

u8 get_log_level() {
	return log_level;
}
