#include "cc_mux.h"
#include "resource.h"
#include "muxer.h"
#include "bits.h"

void mux(void) {
	char tmpStr[256];
	unsigned long i, j, k;
	unsigned char CCpattern, CCextra, CCframes;
	size_t n;
	BOOL gop_started = FALSE;

	init_getbits(mpgin_file);
	outfp = fopen(mpgout_file, "wb");
	if (!outfp) {
		sprintf(userMessage, "Unable to open %s for output.", mpgout_file);
		PrintMessage(TO_ERR);
		KillThread(FALSE);
		return;
	}
	binfp = fopen(cc1bin_file, "rb");
	if (!binfp) {
		sprintf(userMessage, "Unable to open %s for input.", cc1bin_file);
		PrintMessage(TO_ERR);
		KillThread(FALSE);
		return;
	}
	// skip first 4 bytes of BIN file
	n = fread(ccbuffer, sizeof(unsigned char), 4, binfp);
	if (n <= 0)
		fclose(binfp);

	do {
		i = getbits(32);
mainloop:
		switch (i) {
			case MPEG_PROGRAM_END_CODE:
				break;
			case GROUP_START_CODE:
				gop_started = TRUE;
				gopidx++;
				break;
			case PICTURE_START_CODE:
				if (gop_started) {
					gop_started = FALSE;
    				// printf("%d,%d ", gopidx, gopcaptions[gopidx]);
					if (gopcaptions[gopidx] > 0) {
						j = gopcaptions[gopidx];
						CCpattern = j >> 7;
						CCextra = j & 0x01;
						CCframes = j & 0x7f;
						CCframes >>= 1;
					} else {
						CCframes = 0;
					}
					if (CCframes == 0) {
						break;
					}
					// Output closed caption packet
					//  (located just before the first picture packet in each GOP)
					write_bytes(USER_DATA_START_CODE, 4);
					write_bytes(DVD_CLOSED_CAPTION, 4);
					// Attribute byte:
					//  2 x GOP size,
					//  plus 0 not to add an extra field,
					//  plus 0x80 to use the pattern Field 1, Field 2
					j = (CCframes*2) + CCextra + (CCpattern * 0x80);
					write1byte(j);
					if (CCextra)
						CCframes++;
					for (j=0; j<CCframes; j++) {
						// first field
						k = (CCpattern == 0) ? 0xFE : 0xFF;
						write1byte(k);
						if ((binfp) && (CCpattern == 1)) {
							n = fread(ccbuffer, sizeof(unsigned char), 2, binfp);
							if (n <= 0)
								fclose(binfp);
						}
						if (CCpattern == 1) {
							if (binfp) {
								// printf("%02x.%02x ", ccbuffer[0], ccbuffer[1]);
								write1byte(ccbuffer[0]);
								write1byte(ccbuffer[1]);
							} else {
								write_bytes(0x8080, 2);
							}
						} else {
							write_bytes(0x8080, 2);
						}
						if (((j+1) == CCframes) && CCextra) {
							continue;
						}
						// second field
						k = (CCpattern == 0) ? 0xFF : 0xFE;
						write1byte(k);
						if ((binfp) && (CCpattern == 0)) {
							n = fread(ccbuffer, sizeof(unsigned char), 2, binfp);
							if (n <= 0)
								fclose(binfp);
						}
						if (CCpattern == 0) {
							if (binfp) {
								write1byte(ccbuffer[0]);
								write1byte(ccbuffer[1]);
							} else {
								write_bytes(0x8080, 2);
							}
						} else {
							write_bytes(0x8080, 2);
						}
					}
				}
				break;
			default:
				if ((i >> 8) != PACKET_START_CODE_PREFIX) {
					// can't use seek_sync because we want to copy everything to outfp
					while ((i & 0xFFFFFF00) != PICTURE_START_CODE) {
						j = i >> 24;
						write1byte(j);
						i <<= 8;
						i &= 0xFFFFFFFF;
						i |= getbits(8);
						if (end_bs()) {
							break;
						}
					}
					if (end_bs()) {
						break;
					}
					goto mainloop;
				}
		}
		write_bytes(i, 4);
	} while ((i != MPEG_PROGRAM_END_CODE) && (!end_bs()));

	if (binfp) {
		fclose(binfp);
	}
	if (outfp) {
		flush_buffer();
		fclose(outfp);
	}
	finish_getbits();
	return;
}

void write_buffer()
{
  if (fwrite(buffer, sizeof(unsigned char), bufidx + 1, outfp) != (unsigned int)bufidx + 1)
  {
	sprintf(userMessage, "Error writing to output file.");
	PrintMessage(TO_ERR);
	KillThread(FALSE);
	return;
  }
  bufidx = -1;
}

void flush_buffer()
{
  if (bufidx >= 0)
    write_buffer();
}

void write_bytes(unsigned int N, int size) {
	unsigned char i;
	int j;
	for (j=size-1; j>=0; j--) {
		i = (unsigned char)(N >> (j * 8));
		buffer[++bufidx] = i & 0xFF;
		if (bufidx == BUFFER_SIZE - 1)
			write_buffer();
	}
}

void write1byte(unsigned int N) {
	unsigned char i;
	i = (unsigned char)N;
	buffer[++bufidx] = i;
	if (bufidx == BUFFER_SIZE - 1) {
		write_buffer();
	}
}

