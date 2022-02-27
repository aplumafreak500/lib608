/*
608.h
part of Luma's EIA-608 Tools
License: GPLv3 or later
(see License.txt)
*/

#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* Typedefs */

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef u8 bool8;
typedef u16 bool16;
typedef u32 bool32;
typedef u64 bool64;

typedef volatile u8 vu8;
typedef volatile s8 vs8;
typedef volatile u16 vu16;
typedef volatile s16 vs16;
typedef volatile u32 vu32;
typedef volatile s32 vs32;
typedef volatile u64 vu64;
typedef volatile s64 vs64;

typedef volatile f32 vf32;
typedef volatile f64 vf64;

/* Everything else */

typedef struct __attribute__((packed, aligned(4))) {
	s32 hours:12; // up to 2047 hours
	u32 minutes:6;
	u32 seconds:6;
	u32 frames:7; // up to 127 fps
	bool32 drop:1;
} timecode;

typedef struct {
	union {
		timecode tc;
		u32 raw;
	} pts;
	unsigned int entry_count;
	u16 entries[];
} scc_entry;

typedef struct {
	u16 major;
	u16 minor;
	u16 revision;
	u16 build;
	char git_rev[10];
} VersionInfo;

extern const timecode default_timecode;
extern const VersionInfo library_version;

// 608.c
s64 tc2int(timecode pts, f64 fps);
timecode int2tc(s64 pts, f64 fps, bool8 drop);
u8 byteswap8(u8 in);
u16 byteswap16(u16 in);
u32 byteswap24(u32 in);
u32 byteswap32(u32 in);
u64 byteswap48(u64 in);
u64 byteswap64(u64 in);
u16 fixParity(u16 in);

// scc.c
scc_entry* ReadSCC(FILE* scc, size_t* length);
u32 WriteSCC(scc_entry* in, size_t* length, FILE* out);
bool8 IsSCCFile(FILE* file);

// raw.c
extern unsigned int MAX_NULLS; // only ReadRaw uses this value
scc_entry* ReadRaw(FILE* raw, size_t* length, f32 fps, timecode start, bool8 drop);
scc_entry* ReadNW4R(FILE* nw4r, size_t* length);
u32 WriteRaw(scc_entry* in, size_t* length, FILE* out, f32 fps, timecode start, timecode end);
u32 WriteNW4R(scc_entry* in, size_t* length, FILE* out, u8 field, bool8 swap);
bool8 IsRawFile(FILE* file);
bool8 IsNW4RFile(FILE* file);
u8 GetNW4RField(FILE* file);
