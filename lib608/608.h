/*
608.h
part of Luma's EIA-608 Tools
License: GPLv3 or later
(see License.txt)
*/

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

u8 debug;