#define _WIN32_IE	0x0603 // defines Internet Explorer version
#ifndef _T
    #if defined(_UNICODE) || defined(UNICODE)
        #define _T(x) L ## x
    #else
        #define _T(x) x
    #endif
#endif

#define ES 0
#define PROGRAM 1
#define TO_ERR 0
#define TO_OUT 1

#include <windows.h>
#include <commctrl.h>
#include <stdio.h> 
#include <io.h>
#include <fcntl.h>

unsigned int CliActive;
char mpgin_file[1024];
char cc1_file[1024];
char cc2_file[1024];
char cc1bin_file[1024];
char cc2bin_file[1024];
char mpgout_file[1024];
int stream_type;
char this_file[2048];
char file_version[10];
char userMessage[255];
FILE *infp;
FILE *mpgoutfp;
FILE *cc1fp;
FILE *cc2fp;
// For progress bar.
__int64 size;
__int64 data_count;

void usage(void);
void PrintMessage(int msgType);
void KillThread(BOOL printDone);
void SetProgress(UINT pct);

