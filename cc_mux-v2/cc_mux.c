/* To Do: add determine stream type, convert .SCC to .BIN */

/* 
 *  CC_MUX Copyright (C) 2006, McPoodle
 *
 *  The GUI code is largely borrowed from DGPulldown by Donald Graft,
 *  while the bits library and much of the code using it is borrowed from
 *  bbMPEG and bbMPEGTools by Brent Beyeler.
 *
 *  CC_MUX is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  CC_MUX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 */

#include "cc_mux.h"
#include "resource.h"
#include "muxer.h"

HWND hWnd;
HWND hM2VIN;
HWND hCC1;
HWND hCC2;
HWND hM2VOUT;
LONG_PTR pOldM2VINProc;
LONG_PTR pOldCC1Proc;
LONG_PTR pOldCC2Proc;
LONG_PTR pOldM2VOUTProc;
HANDLE hThread;
DWORD threadId;
DWORD WINAPI process(LPVOID n);

BOOL get_version()
{
    DWORD dwHandle;
    DWORD dwDataSize = GetFileVersionInfoSize((LPTSTR)this_file, &dwHandle);
    if (dwDataSize == 0)
    {
        return FALSE;
    }
    LPBYTE lpVersionData[dwDataSize];
    if (!GetFileVersionInfo((LPTSTR)this_file, dwHandle, dwDataSize, 
        (void**)lpVersionData) )
    {
        return FALSE;
    }
    UINT nSize;
    LPVOID lpData;
    if (!VerQueryValue((void **)lpVersionData,
        _T("\\StringFileInfo\\040904b0\\FileVersion"),
        &lpData, &nSize) )
    {
        return FALSE;
    }
    strcpy(file_version, (char*)lpData);
    return TRUE;
}

void OpenAFile(int wID)
{
	char fileName[1024];
	OPENFILENAME ofn;
	fileName[0] = 0;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpTemplateName = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = 1024;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	
	switch (wID)
	{
		case IDC_M2V_IN:
		{
			ofn.lpstrFilter = "Video Stream (*.m1v;*.m2v;*.mpv;\
*.vbs;*.mpg;*.mpeg;*.m2p)\0*.m1v;*.m2v;*.mpv;*.vbs;*.mpg;*.mpeg;*.m2p\0\
All Files (*.*)\0*.*\0";
			ofn.lpstrTitle = "Choose Input MPEG File";
			ofn.lpstrDefExt = "m2v";
			break;
		}
		case IDC_CC_IN1:
		{
			ofn.lpstrFilter = "Scenarist Closed Caption File (*.scc;\
*.sc2)\0*.scc;*.sc2\0Raw Closed Caption File (*.*)\0*.*\0";
			ofn.lpstrTitle = "Choose Field 1 CC File";
			ofn.lpstrDefExt = "bin";
			break;
		}
		case IDC_CC_IN2:
		{
			ofn.lpstrFilter = "Scenarist Closed Caption File (*.scc;\
*.sc2)\0*.scc;*.sc2\0Raw Closed Caption File (*.*)\0*.*\0";
			ofn.lpstrTitle = "Choose Field 2 CC File";
			ofn.lpstrDefExt = "bin";
			break;
		}
		case IDC_M2V_OUT:
		{
			ofn.lpstrFilter = "Video Stream (*.m1v;*.m2v;*.mpv;\
*.vbs;*.mpg;*.mpeg;*.m2p)\0*.m1v;*.m2v;*.mpv;*.vbs;*.mpg;*.mpeg;*.m2p\0\
All Files (*.*)\0*.*\0";
			ofn.lpstrTitle = "Choose Output MPEG File";
			ofn.lpstrDefExt = "m2v";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
				OFN_HIDEREADONLY;
			break;
		}
	}

	if (wID == IDC_M2V_OUT)
	{
		GetSaveFileName(&ofn);
	}
	else
	{
		GetOpenFileName(&ofn);
	}
	if(fileName[0] != 0)
	{
		SetWindowText(GetDlgItem(hWnd, wID), fileName);
		switch(wID)
		{
			case IDC_M2V_IN:
			{
				strcpy(mpgin_file, fileName);
				break;
			}
			case IDC_CC_IN1:
			{
				strcpy(cc1_file, fileName);
				break;
			}
			case IDC_CC_IN2:
			{
				strcpy(cc2_file, fileName);
				break;
			}
			case IDC_M2V_OUT:
			{
				strcpy(mpgout_file, fileName);
				break;
			}
		}
	}	
}

static void HandleDrop(HDROP drop, HWND hwnd)
{
    int count;
    char buf[MAX_PATH];

    count = DragQueryFile(drop, (UINT)-1, buf, MAX_PATH);

    if (count > 1)
	MessageBox(hWnd, "Multiple files dropped - only one file will be processed", "Warning", MB_OK | MB_ICONWARNING);

    if (DragQueryFile(drop, 0, buf, MAX_PATH) == 0) {
	MessageBox(hWnd, "Unable to get dropped file information", "Error", MB_OK | MB_ICONSTOP);
	return;
    }

    SetWindowText(hwnd, buf);
}

static LRESULT CALLBACK EditTextWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
	    case WM_DROPFILES:
	        HandleDrop((HDROP)wp, hwnd);
	        break;
		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDC_M2V_IN:
				    GetDlgItemText(hWnd, IDC_M2V_IN, mpgin_file, 1024);
					break;
				case IDC_CC_IN1:
				    GetDlgItemText(hWnd, IDC_CC_IN1, cc1_file, 1024);
					break;
				case IDC_CC_IN2:
				    GetDlgItemText(hWnd, IDC_CC_IN2, cc2_file, 1024);
					break;
				case IDC_M2V_OUT:
				    GetDlgItemText(hWnd, IDC_M2V_OUT, mpgout_file, 1024);
					break;
			}
			break;
	    default:
	        if (hwnd == hM2VIN)
	        {
	            return CallWindowProc((WNDPROC)pOldM2VINProc, hwnd, msg, wp, lp);
	        }
	        if (hwnd == hCC1)
	        {
	            return CallWindowProc((WNDPROC)pOldCC1Proc, hwnd, msg, wp, lp);
        	}
        	if (hwnd == hCC2)
        	{
            	return CallWindowProc((WNDPROC)pOldCC2Proc, hwnd, msg, wp, lp);
        	}
        	if (hwnd == hM2VOUT)
        	{
            	return CallWindowProc((WNDPROC)pOldM2VOUTProc, hwnd, msg, wp, lp);
        	}
			break;
    }
	return TRUE;
}

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HICON hicon;
	int wID;
    char title[255];
    strcpy(title, "CC_MUX (v.");
    if (get_version())
    {
        strcat(title, file_version);
    }
    else
    {
        strcat(title, "unknown");
    }
    strcat(title, ") by McPoodle");
    switch(Message)
    {
		case WM_INITDIALOG:
			hWnd = hwnd;
			hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ICON)); 
			SendMessage(hwnd, (UINT) WM_SETICON, (WPARAM) ICON_SMALL, (LPARAM) hicon); 
			SendMessage(hwnd, (UINT) WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hicon); 
			SendMessage(hwnd, (UINT) WM_SETTEXT, (WPARAM) 0, (LPARAM)(LPCTSTR)title);
			SetDlgItemText(hwnd, IDC_STATUS, "Output M2V file name defaults to input M2V name plus \".CC.m2v\".");
			hM2VIN = GetDlgItem(hwnd, IDC_M2V_IN);
			hCC1 = GetDlgItem(hwnd, IDC_CC_IN1);
			hCC2 = GetDlgItem(hwnd, IDC_CC_IN2);
			hM2VOUT = GetDlgItem(hwnd, IDC_M2V_OUT);
			pOldM2VINProc = SetWindowLongPtr(hM2VIN, GWL_WNDPROC, (LONG)(LONG_PTR)EditTextWindowProc);
			pOldCC1Proc = SetWindowLongPtr(hCC1, GWL_WNDPROC, (LONG)(LONG_PTR)EditTextWindowProc);
			pOldCC2Proc = SetWindowLongPtr(hCC2, GWL_WNDPROC, (LONG)(LONG_PTR)EditTextWindowProc);
			pOldM2VOUTProc = SetWindowLongPtr(hM2VOUT, GWL_WNDPROC, (LONG)(LONG_PTR)EditTextWindowProc);
			break;
		case WM_COMMAND:
			wID = LOWORD(wParam);
			switch(wID)
			{
				case IDC_GET_M2V_IN:
				{
					OpenAFile(IDC_M2V_IN);
					break;
				}
				case IDC_GET_CC_IN1:
				{
					OpenAFile(IDC_CC_IN1);
					break;
				}
				case IDC_GET_CC_IN2:
				{
					OpenAFile(IDC_CC_IN2);
					break;
				}
				case IDC_GET_M2V_OUT:
				{
					OpenAFile(IDC_M2V_OUT);
					break;
				}
				case IDC_MUX:
				    GetDlgItemText(hWnd, IDC_M2V_IN, mpgin_file, 1024);
				    GetDlgItemText(hWnd, IDC_CC_IN1, cc1_file, 1024);
				    GetDlgItemText(hWnd, IDC_CC_IN2, cc2_file, 1024);
				    GetDlgItemText(hWnd, IDC_M2V_OUT, mpgout_file, 1024);
					SetDlgItemText(hwnd, IDC_STATUS, "Processing, please wait...");
					hThread = CreateThread(NULL, 32000, process, 0, 0, &threadId);
					break;                
			}
			break;
		case WM_CLOSE:
			SetWindowLongPtr(hM2VIN, GWL_WNDPROC, (LONG)(LONG_PTR)pOldM2VINProc);
			SetWindowLongPtr(hCC1, GWL_WNDPROC, (LONG)(LONG_PTR)pOldCC1Proc);
			SetWindowLongPtr(hCC2, GWL_WNDPROC, (LONG)(LONG_PTR)pOldCC2Proc);
			SetWindowLongPtr(hM2VOUT, GWL_WNDPROC, (LONG)(LONG_PTR)pOldM2VOUTProc);
			EndDialog(hwnd, 0);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void usage(void)
{
	char c[2];
	fprintf(stdout, "CC_MUX");
    if (get_version())
    {
		fprintf(stdout, " Version ");
		fprintf(stdout, file_version);
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "  Inserts closed captions in raw format into an MPEG file\n");
	fprintf(stdout, "    (Video Elementary Stream, not Program Stream).\n");
	fprintf(stdout, "  Syntax: CC_MUX infile1.m2v infile2.scc -2 infile3.scc outfile.m2v\n");
	fprintf(stdout, "    infile1.???: MPEG 1 or 2 video file to process (any of various extensions)\n");
	fprintf(stdout, "    infile2.scc: Scenarist Closed Caption file to insert for Field 1\n");
	fprintf(stdout, "    -2 infile3.scc (OPTIONAL): SCC file to insert for Field 2\n");
	fprintf(stdout, "    outfile.??? (OPTIONAL): name of MPEG file to output\n");
	fprintf(stdout, "         (DEFAULT: infile1.???.CC.m2v)\n\n");
	fprintf(stdout, "Press Enter to continue...");
	fgets(c, 2, stdin);
	return;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc = __argc;
	char **argv = __argv;
	int i;
	
	CliActive = 0;
	cc2fp = NULL;
	
	strcpy(this_file, argv[0]);
	if (strchr(this_file, '.') == NULL)
		strcat(this_file, ".exe");
	
	if (argc > 1)
	{
		if (AllocConsole())
		{
			freopen("CONIN$", "rb", stdin);
			freopen("CONOUT$", "wb", stdout);
			freopen("CONOUT$", "wb", stderr);
		}
		else
			return 1;
		if (argc < 3)
		{
			usage();
			return 1;
		}
		CliActive = 1;
		for (i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-2") == 0)
			{
				if (argc > i)
				{
					strncpy(cc2_file, argv[i+1], sizeof(cc2_file));
					// gobble up the argument
					i++;
					continue;
				}
				else
				{
					usage();
					return 1;
				}
			}
			if (mpgin_file[0] == 0)
			{
				strncpy(mpgin_file, argv[i], sizeof(mpgin_file));
				continue;
			}
			if (cc1_file[0] == 0)
			{
				strncpy(cc1_file, argv[i], sizeof(mpgin_file));
				continue;
			}
			if (mpgout_file[0] == 0)
			{
				strncpy(mpgout_file, argv[i], sizeof(mpgout_file));
				continue;
			}
			// if you've gotten here, then you entered too many arguments
			usage();
			return 1;
		}
		fprintf(stdout, "Processing, please wait...\n");
		hThread = CreateThread(NULL, 32000, process, 0, 0, &threadId);
		WaitForSingleObject(hThread, INFINITE);
		return 0;
	}
	else
	{
		INITCOMMONCONTROLSEX InitCtrlEx;
		InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
		InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
		InitCommonControlsEx(&InitCtrlEx);
		return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);
	}
}

void PrintMessage(int msgType)
{
	char c[2];
	if (CliActive)
	{
		strcat(userMessage, "\n");
		if (msgType == TO_ERR)
		{
			fprintf(stderr, userMessage);
			fprintf(stdout, "Press Enter to continue...");
			fgets(c, 2, stdin);
		} else
		{
			fprintf(stdout, userMessage);
		}
	}
	else
	{
		SetDlgItemText(hWnd, IDC_STATUS, userMessage);
	}
}

void KillThread(BOOL printDone)
{
	if (printDone)
	{
		strcpy(userMessage, "Done.");
		PrintMessage(TO_OUT);
	}
	ExitThread(0);
}

void InitProgress(void);

DWORD WINAPI process(LPVOID n)
{
	int fd;

	// Test to see that the arguments are files.
	if (mpgin_file[0] == 0 || (access(mpgin_file, R_OK) == -1))
	{
		strcpy(userMessage, "Could not open the input M2V file!");
		PrintMessage(TO_ERR);
		KillThread(FALSE);
		return 1;
	}
	if (mpgout_file[0] == 0)
	{
		strcpy(mpgout_file, mpgin_file);
		strcat(mpgout_file, ".CC.m2v");
	}
	if ((mpgoutfp = fopen(mpgout_file, "wb")) == NULL)
	{
		strcpy(userMessage, "Could not open the output M2V file!");
		PrintMessage(TO_ERR);
		KillThread(FALSE);
		return 1;
	}
	else
	{
		fclose(mpgoutfp);
		if (remove(mpgout_file) != 0)
		{
			strcpy(userMessage, "Could not delete file!");
			PrintMessage(TO_ERR);
			KillThread(FALSE);
			return 1;
		}
	}
	if (cc1_file[0] == 0 || (access(cc1_file, R_OK) == -1))
	{
		strcpy(userMessage, "Could not open CC file 1!");
		PrintMessage(TO_ERR);
		KillThread(FALSE);
		return 1;
	}
	if (cc2_file[0] != 0)
	{
		if (access(cc2_file, R_OK) == -1)
		{
			strcpy(userMessage, "Could not open CC file 2!");
			PrintMessage(TO_ERR);
			KillThread(FALSE);
			return 1;
		}
	}
	// Get the file size.
	fd = _open(mpgin_file, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
	size = _lseeki64(fd, 0, SEEK_END);
	_close(fd);
	InitProgress();

	mux();
	KillThread(TRUE);
	return 0;
}

void InitProgress(void)
{
	unsigned int i;
	if (CliActive)
	{
		fprintf(stdout, "<");
		for (i = 0; i < 100; i += 2)
		{
			fprintf(stdout, " ");
		}
		fprintf(stdout, ">");
	}
	else
	{
		SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, (UINT) 0, 0);
	}
}

void SetProgress(UINT pct)
{
	unsigned int i;
	if (CliActive)
	{
		fprintf(stdout, "\b");
		for (i = 0; i < 100; i += 2)
		{
			fprintf(stdout, "\b");
		}
		for (i = 0; i < 100; i += 2)
		{
			if (i < pct)
				fprintf(stdout, ".");
			else
				fprintf(stdout, " ");
		}
		fprintf(stdout, ">");
	}
	else
	{
		SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, pct, 0);
	}
}

