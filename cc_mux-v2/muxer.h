void determine_stream_type(void);
#include <stdlib.h>
#include <string.h>
//#include <iostream.h>

#define MPEG_PROGRAM_END_CODE    0x000001B9
#define GROUP_START_CODE         0x000001B8
#define EXTENSION_START_CODE     0x000001B5
#define PICTURE_START_CODE       0x00000100
#define USER_DATA_START_CODE     0x000001B2

#define DVD_CLOSED_CAPTION       0x434301F8
#define PACKET_START_CODE_PREFIX 0x000001
#define PICTURE_CODING_EXTENSION 0x04

#define BUFFER_SIZE              32768

static unsigned char gopframes[BUFFER_SIZE];
static unsigned char gopcaptions[BUFFER_SIZE];
static unsigned char buffer[BUFFER_SIZE];
static unsigned char ccbuffer[16];
static int gopidx;
static int bufidx;
static FILE *outfp;
static FILE *binfp;

void usage();
void mux(void);
void write_buffer();
void flush_buffer();
void write_bytes(unsigned int N, int size);
void write1byte(unsigned int N);

