/*
log.h
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#define LOG_FATAL 0x1
#define LOG_ERROR 0x2
#define LOG_WARN 0x4
#define LOG_INFO 0x8
#define LOG_DEBUG 0x10
#define LOG_TRACE 0x20
#define LOG_LIBRARY 0x40
#define LOG_APPLICATION 0x80

#define LOG_DEFAULT LOG_FATAL | LOG_ERROR | LOG_WARN | LOG_INFO | LOG_APPLICATION | LOG_LIBRARY
#define LOG_ALL 0xff
#define LOG_SILENT 0
#define LOG_VERBOSE LOG_DEFAULT | LOG_DEBUG

extern bool8 use_colors;

int log_write(u8 level, bool8 color, char* fmt, ...);
u8 change_log_level(u8 newLevel);
u8 reset_log_level();
u8 get_log_level();
