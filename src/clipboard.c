#include <windows.h>
#include <libiberty/libiberty.h>

#ifdef UNICODE
 #define CF_TTEXT CF_UNICODETEXT
#else
 #define CF_TTEXT CF_TEXT
#endif

static
size_t _strSize(LPCTSTR str) {
	return (lstrlen(str)+1)*sizeof(TCHAR); }

static
LPTSTR _strDup(LPCTSTR str)
{
#ifdef UNICODE
	size_t size = _strSize(str);
	return xmemdup(str, size, size);
#else
	return xstrdup(str);
#endif
}

LPTSTR clipBoad_GetText(void)
{
	while(!OpenClipboard(NULL)) Sleep(100);
	HANDLE hMem = GetClipboardData(CF_TTEXT);
	LPTSTR str = _strDup((LPTSTR)GlobalLock(hMem));
	GlobalUnlock(hMem);
	CloseClipboard();
	return str;
}

void clipBoad_SetText(LPCTSTR str)
{
	if(str == NULL)	return;
	size_t size = _strSize(str);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	memcpy(GlobalLock(hMem), str, size);
	GlobalUnlock(hMem);
	while(!OpenClipboard(0)) Sleep(100);
	EmptyClipboard();
	SetClipboardData(CF_TTEXT, hMem);
	CloseClipboard();
}

