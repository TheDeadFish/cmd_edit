#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <assert.h>
#include <windows.h>
#include <libiberty/libiberty.h>
#include "resource.h"

LPTSTR clipBoad_GetText(void);
void clipBoad_SetText(LPCTSTR str);

static WNDPROC editCtrl_wndProc;
static LRESULT CALLBACK editCtrl_subProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = editCtrl_wndProc(hwnd, msg, wParam, lParam);
	if(msg == WM_GETDLGCODE) result &= ~DLGC_HASSETSEL;
	return result;
}

static
void editCtrl_subClass(HWND hwnd, UINT ctrlId)
{
	hwnd = GetDlgItem(hwnd, ctrlId);
	editCtrl_wndProc = (WNDPROC)SetWindowLongPtr(hwnd,
		GWLP_WNDPROC, (LONG_PTR)editCtrl_subProc);
}


BOOL change_flag;
TCHAR srcBuff[65536];
TCHAR dstBuff[65536];


void __fastcall decode_command(TCHAR* dst, TCHAR* src)
{
NEXT_LINE:
	while(*src && *src <= ' ') src++;
	if(*src && (dst != dstBuff)) {
		*dst++ = '\r'; *dst++ = '\n'; }

	BOOL inQuote = FALSE;
	for(; (*dst = *src); src++)
	{
		if(*src == '"') {
			inQuote = !inQuote;
			continue;
		}

		if((*src <= ' ')
		&&(inQuote == FALSE))
			goto NEXT_LINE;

		*dst = *src;
		dst++;
	}
}

void __fastcall encode_command(TCHAR* dst, TCHAR* src)
{
	while(1) {
		if(*src == '\n') src++;
		if(*src == 0) break;

		// locate end of line
		BOOL hasQuote = FALSE;
		TCHAR* end = src;
		for(;*end && (*end != '\n'); end++) {
			if(*end == ' ') hasQuote = TRUE; }

		// copy the line
		if(src != srcBuff) *dst++ = ' ';
		if(hasQuote) *dst++ = '"';
		while(src < end) *dst++ = *src++;
		if(hasQuote) *dst++ = '"';
	}

	*dst = 0;
}

void encode_decode(HWND hwnd, UINT ctrlId)
{
	if(change_flag) return;
	change_flag = TRUE;
	GetDlgItemText(hwnd, ctrlId, srcBuff, ARRAYSIZE(srcBuff));

	TCHAR* dst = dstBuff; TCHAR* src = srcBuff;
	if(ctrlId == edt1) {
		decode_command(dst, src);
	} else {
		encode_command(dst, src);
	}

	SetDlgItemText(hwnd, ctrlId^1, dstBuff);
	change_flag = FALSE;
}

void clipBoard_copy(HWND hwnd)
{
	GetDlgItemText(hwnd, edt1, srcBuff, ARRAYSIZE(srcBuff));
	clipBoad_SetText(srcBuff);
}

void clipBoard_paste(HWND hwnd)
{
	LPTSTR buff = clipBoad_GetText();
	SetDlgItemText(hwnd, edt1, buff);
	free(buff);
}

void append_string(HWND hwnd, LPTSTR str)
{
	/* append new line */
	LPTSTR end = str+lstrlen(str);
	DWORD prevEnd = *(DWORD*)(end+1);
	lstrcpy(end, TEXT("\r\n"));

	/* convert path to unix */
	if(IsDlgButtonChecked(hwnd, UNIX_MODE)) {
		if(str[0] && str[1] == ':')
			str[1] = str[0]; str[0] = '/';
		for(size_t i = 0; str[i]; i++) {
			if(str[i] == '\\') str[i] = '/'; }
	}

	SendDlgItemMessage(hwnd, edt2, EM_REPLACESEL, FALSE, (LPARAM)str);

	*(DWORD*)(end+1) = prevEnd;
}


void openFiles(HWND hwnd)
{
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = srcBuff;
	ofn.nMaxFile = 65536;
	ofn.Flags |= OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lpstrFile[0] = 0;

	if(GetOpenFileName(&ofn)) {
		assert(ofn.nFileOffset != 0);
		TCHAR* src = ofn.lpstrFile+ofn.nFileOffset;
		src[-1] = '\\';

		change_flag = TRUE;
		while(*src) {
			TCHAR* dst = ofn.lpstrFile+ofn.nFileOffset;
			while(*dst++ = *src++);
			append_string(hwnd, ofn.lpstrFile);
		}

		change_flag = FALSE;
		encode_decode(hwnd, edt2);
	}
}

void openPath(HWND hwnd)
{
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = srcBuff;
	ofn.nMaxFile = 65536;
	ofn.Flags |= OFN_EXPLORER;
	ofn.lpstrFile[0] = 0;
	lstrcpy(srcBuff, TEXT("dummy_file"));

	if(GetSaveFileName(&ofn)) {
		ofn.lpstrFile[ofn.nFileOffset] = 0;
		append_string(hwnd, ofn.lpstrFile);
	}
}

void initDialog(HWND hwnd)
{
	editCtrl_subClass(hwnd, edt1);
	editCtrl_subClass(hwnd, edt2);
}

INT_PTR WINAPI Dlgproc(
  HWND hwnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
) {
	switch(uMsg) {
	case WM_INITDIALOG:
		initDialog(hwnd);
		break;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		break;

	case WM_COMMAND:
		switch(wParam) {
		case GET_FILE:
			openFiles(hwnd);
			break;
		case GET_PATH:
			openPath(hwnd);
			break;
		case COPY:
			clipBoard_copy(hwnd);
			break;
		case PASTE:
			clipBoard_paste(hwnd);
			break;
		case MAKEWPARAM(edt1, EN_CHANGE):
		case MAKEWPARAM(edt2, EN_CHANGE):
			encode_decode(hwnd, LOWORD(wParam));
			break;
		}

		break;

	default:
		return FALSE;
	}

	return TRUE;
}

int main()
{
	DialogBox(NULL,
		MAKEINTRESOURCE(IDD_MAINDLG),
		NULL, Dlgproc);
	return 0;
}
