/*
608.c
part of Luma's EIA-608 Tools
License: GPL v3 or later
(see License.txt)
*/

#include <math.h> // fmod, for int2tc

#include "608.h"
#include "config.h" // for git
#include "log.h"

const timecode default_timecode = {0, 0, 0, 0, false};
const VersionInfo library_version = {0, 5, 0, 1, VERSION};

// no idea why the 1/2 frame is needed but idk
s64 tc2int(timecode pts, f32 fps) {
	f32 ret;
	if (pts.drop) {
		ret = (((60*60*fps)-108)*pts.hours)+(((60*10*fps)-18)*(pts.minutes/10.0f))+(((60*fps)-2)*(fmodf(pts.minutes, 10)))+(fps*pts.seconds)+pts.frames+0.5f;
	}
	else {
		ret = (60*60*fps*pts.hours)+(60*fps*pts.minutes)+(fps*pts.seconds)+pts.frames+0.5f;
	}
	log_write(LOG_TRACE, use_colors, "tc2int: %02d:%02hhu:%02hhu%c%02hhu -> 0x%08x\n", pts.hours, pts.minutes, pts.seconds, pts.drop ? ';' : ':', pts.frames, (s64) ret);
	return (s64) ret;
}
timecode int2tc(s64 pts, f32 fps, bool8 drop) {
	timecode ret = {0};
	f32 _pts = (f32) pts; // for logging pruposes
	// McPoodle's tools fail on negative pts; however, NW4R format allows for it so properly handle it
	f32 hours, minutes, seconds, frames, dm; // To allow operations with the fps value
	if (drop) {
		hours = (_pts+108)/(60*60*fps);
		if (_pts < 0) {
			_pts *= -1;
		}
		_pts = fmodf(_pts+108, 60*60*fps);
		dm = (_pts+18)/(60*10*fps);
		_pts = fmodf(_pts+18, 60*10*fps);
		minutes = dm+((_pts+2)/(60*fps));
		_pts = fmodf(_pts+2, 60*fps);
		seconds = _pts/fps;
		frames = fmodf(_pts, fps) + 0.5f; // why?
		if ((fmodf(minutes, 10) > 0) && (frames < 2)) {
			frames = 2;
		}
	}
	else {
		hours = _pts/(60*60*fps);
		if (_pts < 0) {
			_pts *= -1;
		}
		_pts = fmodf(_pts, 60*60*fps);
		minutes = _pts/(60*fps);
		_pts = fmodf(_pts, 60*fps);
		seconds = _pts/fps;
		frames = fmodf(_pts, fps) + 0.5f; // why?
	}
	// Correct invalid timestamps
	// TODO: multi-pass
	if (frames > fps) {
		seconds++;
		frames = frames - fps;
	}
	if (seconds > 59) {
		minutes++;
		seconds = seconds - 60;
	}
	if (minutes > 59) {
		hours++;
		minutes = minutes - 60;
	}
	ret.hours = (s16) hours;
	ret.minutes = (u8) ((int) minutes & 0x3f);
	ret.seconds = (u8)	((int) seconds & 0x3f);
	ret.frames = (u8) ((int) frames & 0x7f);
	ret.drop = (u8) (drop & 0x1);
	log_write(LOG_TRACE, use_colors, "int2tc: 0x%08x -> %02d:%02hhu:%02hhu%c%02hhu\n", (s32) pts, ret.hours, ret.minutes, ret.seconds, ret.drop ? ';' : ':', ret.frames); // the cast is needed as 64 bit types won't work right with printf for some reason or another
	return ret;
}
u8 byteswap8(u8 in) {
	return in; // nop
}
u16 byteswap16(u16 in) {
	return (u16)((in & 0xff) << 8) | ((in & 0xff00) >> 8);
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
