/*
version.h
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

typedef struct {
	u16 major;
	u16 minor;
	u16 revision;
	u16 build;
	char git_rev[10];
} VersionInfo;
