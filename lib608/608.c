/*
608.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include "608.h"
#include "config.h" // for git
#include "log.h"

const timecode default_timecode = {0, 0, 0, 0, false};
const VersionInfo library_version = {0, 1, 0, 0xc0, ""};

// no idea why the 1/2 frame is needed but idk
s64 tc2int(timecode pts, f32 fps) {
	s64 ret;
	if (pts.drop) {
		ret = (((60*60*fps)-108)*pts.hours)+(((60*10*fps)-18)*(pts.minutes/10))+(((60*fps)-2)*(pts.minutes%10))+(fps*pts.seconds)+pts.frames+0.5f;
	}
	else {
		ret = (60*60*fps*pts.hours)+(60*fps*pts.minutes)+(fps*pts.seconds)+pts.frames+0.5f;
	}
	log_write(LOG_TRACE, use_colors, "tc2int: %02d:%02hhu:%02hhu%c%02hhu -> 0x%08x\n", pts.hours, pts.minutes, pts.seconds, pts.drop ? ';' : ':', pts.frames, ret);
	return ret;
}
timecode int2tc(s64 pts, f32 fps, bool8 drop) {
	return default_timecode; //TODO
}
u8 byteswap8(u8 in) {
	return in; // noop
}
u16 byteswap16(u16 in) {
	return ((in & 0xff) << 8) | ((in & 0xff00) >> 8);
}
u32 byteswap24(u32 in) {
	return ((in & 0xff) << 16) | ((in & 0xff0000) >> 16);
}
u32 byteswap32(u32 in) {
	return ((in & 0xff) << 24) | ((in & 0xff00) << 8) | ((in & 0xff0000) >> 8) | ((in & 0xff000000) >> 24);
}
u64 byteswap48(u64 in) {
	return ((in & 0xff) << 40) | ((in & 0xff00) << 24) | ((in & 0xff0000) << 8) | ((in & 0xff000000) >> 8) | ((in & 0xff00000000) >> 24) | ((in & 0xff0000000000) >> 40);
}
u64 byteswap64(u64 in) {
	return ((in & 0xff) << 56) | ((in & 0xff00) << 40) | ((in & 0xff0000) << 24) | ((in & 0xff000000) << 8) | ((in & 0xff00000000) >> 8) | ((in & 0xff0000000000) >> 24) | ((in & 0xff000000000000) >> 40) | ((in & 0xff00000000000000) >> 56);
}

u16 fixParity(u16 in) {
	const bool8 odd[128] = {
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, // 00
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, // 10
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, // 20
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, // 30
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //40
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, // 50
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, // 60
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0  // 70
	};
	u16 out = in;
	if (odd[in & 0x7f]) {
		out |= 0x80;
	}
	if (odd[(in & 0x7f00) >> 8]) {
		out |= 0x8000;
	}
	log_write(LOG_TRACE, use_colors, "FixParity: in %04x, out %04x\n", in, out);
	return out;
}
