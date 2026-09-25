// Minimal MFC/Win32 shim for Decker (SDL2 frontend, desktop + web).
// Only what Decker uses. Windows draw into per-top-level backing bitmaps;
// the frontend (port/fe_sdl.cpp) composites them into one 640x480 screen.
#ifndef __AFXWIN_H__
#define __AFXWIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------- types
typedef int BOOL;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int UINT;
typedef int32_t LONG;
typedef intptr_t LPARAM;
typedef uintptr_t WPARAM;
typedef intptr_t LRESULT;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef uintptr_t DWORD_PTR;
typedef DWORD COLORREF;
typedef char TCHAR;
typedef const char *LPCTSTR, *LPCSTR;
typedef char *LPTSTR, *LPSTR;
typedef void *LPVOID;
typedef BYTE *LPBYTE;
typedef DWORD *LPDWORD;
typedef long HKEY;
typedef void *HINSTANCE, *HICON, *HCURSOR, *HBRUSH, *HGDIOBJ, *HPALETTE, *HFONT, *HPEN, *HANDLE;
typedef int HACCEL;
typedef struct HMENU__ *HMENU;
class CWnd;
class CDC;
typedef CWnd *HWND;
typedef CDC *HDC;
struct Bitmap;
typedef Bitmap *HBITMAP;
struct POSITION__;
typedef POSITION__ *POSITION;

#define TRUE 1
#define FALSE 0
#define CALLBACK
#define WINAPI
#define AFX_MSG_CALL
#define afx_msg
#define AFXAPI
#define PASCAL
#define _T(x) x
#define MAX_PATH 260
#define ASSERT(x) ((void)0)
#define VERIFY(x) ((void)(x))
#define TRACE0(x) ((void)0)
#define TRACE(...) ((void)0)
#define UNUSED_ALWAYS(x) ((void)(x))

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g)) << 8) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)((c) >> 8))
#define GetBValue(c) ((BYTE)((c) >> 16))
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)((DWORD_PTR)(l) >> 16))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define MAKEINTRESOURCE(i) ((LPSTR)(uintptr_t)(WORD)(i))
#define IS_INTRESOURCE(p) (((uintptr_t)(p)) >> 16 == 0)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#define __max(a, b) (((a) > (b)) ? (a) : (b))
#define _inline inline
#define __inline inline
#define max(a, b) (((a) > (b)) ? (a) : (b))

struct tagPOINT { LONG x, y; };
typedef tagPOINT POINT, *LPPOINT;
struct tagSIZE { LONG cx, cy; };
typedef tagSIZE SIZE, *LPSIZE;
struct tagRECT { LONG left, top, right, bottom; };
typedef tagRECT RECT, *LPRECT;
typedef const RECT *LPCRECT;

class CPoint : public tagPOINT {
public:
	CPoint() { x = y = 0; }
	CPoint(int ix, int iy) { x = ix; y = iy; }
	CPoint(POINT p) { x = p.x; y = p.y; }
	BOOL operator==(POINT p) const { return x == p.x && y == p.y; }
	BOOL operator!=(POINT p) const { return !(*this == p); }
	CPoint operator+(POINT p) const { return CPoint(x + p.x, y + p.y); }
	CPoint operator-(POINT p) const { return CPoint(x - p.x, y - p.y); }
	void operator+=(POINT p) { x += p.x; y += p.y; }
	void Offset(int dx, int dy) { x += dx; y += dy; }
};
class CSize : public tagSIZE {
public:
	CSize() { cx = cy = 0; }
	CSize(int x, int y) { cx = x; cy = y; }
	CSize(SIZE s) { cx = s.cx; cy = s.cy; }
	BOOL operator==(SIZE s) const { return cx == s.cx && cy == s.cy; }
};
class CRect : public tagRECT {
public:
	CRect() { left = top = right = bottom = 0; }
	CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	CRect(LPCRECT r) { left = r->left; top = r->top; right = r->right; bottom = r->bottom; }
	CRect(const RECT &r) { left = r.left; top = r.top; right = r.right; bottom = r.bottom; }
	CRect(POINT p, SIZE s) { left = p.x; top = p.y; right = p.x + s.cx; bottom = p.y + s.cy; }
	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	CSize Size() const { return CSize(Width(), Height()); }
	CPoint &TopLeft() { return *(CPoint *)&left; }
	CPoint &BottomRight() { return *(CPoint *)&right; }
	void CopyRect(LPCRECT r) { *(RECT *)this = *r; }
	void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
	void DeflateRect(int dx, int dy) { left += dx; right -= dx; top += dy; bottom -= dy; }
	void InflateRect(int dx, int dy) { DeflateRect(-dx, -dy); }
	BOOL PtInRect(POINT p) const { return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }
	BOOL IsRectEmpty() const { return right <= left || bottom <= top; }
	operator LPRECT() { return this; }
	operator LPCRECT() const { return this; }
};

// ---------------------------------------------------------------- constants
#define IDOK 1
#define IDCANCEL 2
#define IDABORT 3
#define IDRETRY 4
#define IDIGNORE 5
#define IDYES 6
#define IDNO 7
#define IDC_STATIC (-1)
#define ID_HELP 0xE146

#define MB_OK 0x0
#define MB_OKCANCEL 0x1
#define MB_YESNOCANCEL 0x3
#define MB_YESNO 0x4
#define MB_ICONHAND 0x10
#define MB_ICONQUESTION 0x20
#define MB_ICONEXCLAMATION 0x30
#define MB_ICONINFORMATION 0x40
#define MB_ICONSTOP MB_ICONHAND
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONWARNING MB_ICONEXCLAMATION

#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_GROUP 0x00020000L
#define WS_TABSTOP 0x00010000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MAXIMIZEBOX 0x00010000L
#define WS_OVERLAPPEDWINDOW 0x00CF0000L
#define WS_EX_CLIENTEDGE 0x00000200L
#define WS_EX_APPWINDOW 0x00040000L
#define WS_EX_STATICEDGE 0x00020000L
#define WS_EX_TRANSPARENT 0x00000020L
#define DS_MODALFRAME 0x80L
#define DS_SETFONT 0x40L
#define DS_CENTER 0x0800L

#define BS_PUSHBUTTON 0x0L
#define BS_DEFPUSHBUTTON 0x1L
#define BS_CHECKBOX 0x2L
#define BS_AUTOCHECKBOX 0x3L
#define BS_RADIOBUTTON 0x4L
#define BS_3STATE 0x5L
#define BS_AUTO3STATE 0x6L
#define BS_GROUPBOX 0x7L
#define BS_AUTORADIOBUTTON 0x9L
#define BS_OWNERDRAW 0xBL
#define BS_TYPEMASK 0xFL
#define BS_LEFTTEXT 0x20L
#define BS_BITMAP 0x80L
#define BS_ICON 0x40L
#define BS_LEFT 0x100L
#define BS_RIGHT 0x200L
#define BS_CENTER 0x300L
#define BS_MULTILINE 0x2000L
#define BS_FLAT 0x8000L

#define SS_LEFT 0x0L
#define SS_CENTER 0x1L
#define SS_RIGHT 0x2L
#define SS_ICON 0x3L
#define SS_BLACKRECT 0x4L
#define SS_BLACKFRAME 0x7L
#define SS_LEFTNOWORDWRAP 0xCL
#define SS_BITMAP 0xEL
#define SS_ETCHEDFRAME 0x12L
#define SS_TYPEMASK 0x1FL
#define SS_NOTIFY 0x100L
#define SS_NOPREFIX 0x80L
#define SS_CENTERIMAGE 0x200L
#define SS_SUNKEN 0x1000L

#define ES_LEFT 0x0L
#define ES_CENTER 0x1L
#define ES_RIGHT 0x2L
#define ES_MULTILINE 0x4L
#define ES_UPPERCASE 0x8L
#define ES_AUTOVSCROLL 0x40L
#define ES_AUTOHSCROLL 0x80L
#define ES_READONLY 0x800L
#define ES_WANTRETURN 0x1000L
#define ES_NUMBER 0x2000L

#define CBS_SIMPLE 0x1L
#define CBS_DROPDOWN 0x2L
#define CBS_DROPDOWNLIST 0x3L
#define CBS_SORT 0x100L
#define CBS_AUTOHSCROLL 0x40L
#define LBS_NOTIFY 0x1L
#define LBS_SORT 0x2L
#define LBS_NOINTEGRALHEIGHT 0x100L
#define LBS_STANDARD 0xA00003L
#define LBS_HASSTRINGS 0x40L
#define LB_ERR (-1)
#define CB_ERR (-1)

#define LVS_ICON 0x0000
#define LVS_REPORT 0x0001
#define LVS_SMALLICON 0x0002
#define LVS_LIST 0x0003
#define LVS_SINGLESEL 0x0004
#define LVS_SHOWSELALWAYS 0x0008
#define LVS_SORTASCENDING 0x0010
#define LVS_SORTDESCENDING 0x0020
#define LVS_NOSCROLL 0x2000
#define LVS_NOCOLUMNHEADER 0x4000
#define LVS_NOSORTHEADER 0x8000
#define LVS_ALIGNLEFT 0x0800
#define LVS_AUTOARRANGE 0x0100
#define LVS_EDITLABELS 0x0200
#define UDS_WRAP 0x1
#define UDS_SETBUDDYINT 0x2
#define UDS_ALIGNRIGHT 0x4
#define UDS_ALIGNLEFT 0x8
#define UDS_AUTOBUDDY 0x10
#define UDS_ARROWKEYS 0x20
#define UDS_HORZ 0x40
#define UDS_NOTHOUSANDS 0x80

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOW 5
#define SW_SHOWNA 8
#define SW_RESTORE 9
#define SWP_NOSIZE 0x1
#define SWP_NOMOVE 0x2
#define SWP_NOZORDER 0x4
#define SWP_SHOWWINDOW 0x40
#define HWND_TOP ((HWND)0)
#define RDW_INVALIDATE 0x1
#define RDW_ERASE 0x4
#define RDW_UPDATENOW 0x100
#define RDW_ALLCHILDREN 0x80
#define RDW_FRAME 0x400

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXVSCROLL 2
#define SM_CYHSCROLL 3
#define SM_CYCAPTION 4
#define SM_CXBORDER 5
#define SM_CYBORDER 6
#define SM_CXFIXEDFRAME 7
#define SM_CYFIXEDFRAME 8
#define SM_CXICON 11
#define SM_CYICON 12
#define SM_CXDLGFRAME SM_CXFIXEDFRAME
#define SM_CYDLGFRAME SM_CYFIXEDFRAME

#define CS_VREDRAW 1
#define CS_HREDRAW 2
#define CS_DBLCLKS 8
#define COLOR_WINDOW 5
#define COLOR_BTNFACE 15
#define IDC_ARROW MAKEINTRESOURCE(32512)
#define IDI_APPLICATION MAKEINTRESOURCE(32512)

#define SRCCOPY 0x00CC0020
#define SRCPAINT 0x00EE0086
#define SRCAND 0x008800C6
#define SRCINVERT 0x00660046
#define NOTSRCCOPY 0x00330008
#define BLACKNESS 0x00000042
#define WHITENESS 0x00FF0062
#define MERGECOPY 0x00C000CA
#define TRANSPARENT 1
#define OPAQUE 2
#define MM_TEXT 1
#define ETO_OPAQUE 0x2
#define ETO_CLIPPED 0x4
#define DT_LEFT 0x0
#define DT_CENTER 0x1
#define DT_RIGHT 0x2
#define DT_VCENTER 0x4
#define DT_WORDBREAK 0x10
#define DT_SINGLELINE 0x20
#define DT_NOPREFIX 0x800
#define DT_CALCRECT 0x400
#define WHITE_PEN 6
#define BLACK_PEN 7
#define NULL_PEN 8
#define WHITE_BRUSH 0
#define BLACK_BRUSH 4
#define NULL_BRUSH 5
#define SYSTEM_FONT 13
#define DEFAULT_GUI_FONT 17
#define ANSI_VAR_FONT 12
#define DST_BITMAP 0x4
#define DSS_NORMAL 0x0
#define DSS_DISABLED 0x20
#define IMAGE_BITMAP 0
#define IMAGE_ICON 1
#define LR_DEFAULTCOLOR 0
#define LR_LOADFROMFILE 0x10
#define LR_COPYRETURNORG 0x4
#define LR_CREATEDIBSECTION 0x2000
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
#define PS_SOLID 0

#define ODT_BUTTON 4
#define ODA_DRAWENTIRE 1
#define ODA_SELECT 2
#define ODA_FOCUS 4
#define ODS_SELECTED 0x1
#define ODS_GRAYED 0x2
#define ODS_DISABLED 0x4
#define ODS_CHECKED 0x8
#define ODS_FOCUS 0x10

#define ILD_NORMAL 0x0
#define ILD_TRANSPARENT 0x1
#define ILD_MASK 0x10
#define ILD_IMAGE 0x20
#define ILD_ROP 0x40
#define ILD_BLEND25 0x2
#define ILD_BLEND50 0x4
#define ILD_FOCUS ILD_BLEND25
#define ILD_SELECTED ILD_BLEND50
#define ILC_MASK 0x1
#define ILC_COLOR 0x0
#define ILC_COLOR4 0x4
#define ILC_COLOR8 0x8
#define ILC_COLOR16 0x10
#define ILC_COLOR24 0x18
#define ILC_COLOR32 0x20
#define CLR_NONE 0xFFFFFFFFL
#define CLR_DEFAULT 0xFF000000L
#define LVSIL_NORMAL 0
#define LVSIL_SMALL 1
#define LVSIL_STATE 2

#define LVCFMT_LEFT 0
#define LVCFMT_RIGHT 1
#define LVCFMT_CENTER 2
#define LVIF_TEXT 0x1
#define LVIF_IMAGE 0x2
#define LVIF_PARAM 0x4
#define LVIF_STATE 0x8
#define LVIS_FOCUSED 0x1
#define LVIS_SELECTED 0x2
#define LVIS_CUT 0x4
#define LVIS_DROPHILITED 0x8
#define LVNI_ALL 0x0
#define LVNI_FOCUSED 0x1
#define LVNI_SELECTED 0x2
#define LVFI_PARAM 0x1
#define LVFI_STRING 0x2
#define LVCF_FMT 0x1
#define LVCF_WIDTH 0x2
#define LVCF_TEXT 0x4
#define LVCF_SUBITEM 0x8

#define TPM_LEFTALIGN 0x0
#define TPM_RIGHTBUTTON 0x2
#define MF_STRING 0x0
#define MF_BYCOMMAND 0x0
#define MF_GRAYED 0x1

#define OBM_UPARROW 32753
#define OBM_DNARROW 32752
#define OBM_RGARROW 32751
#define OBM_LFARROW 32750
#define OBM_UPARROWD 32743
#define OBM_DNARROWD 32742
#define OBM_UPARROWI 32737
#define OBM_DNARROWI 32736

#define OFN_READONLY 0x1
#define OFN_OVERWRITEPROMPT 0x2
#define OFN_HIDEREADONLY 0x4
#define OFN_PATHMUSTEXIST 0x800
#define OFN_FILEMUSTEXIST 0x1000

#define SND_SYNC 0x0
#define SND_ASYNC 0x1
#define SND_NODEFAULT 0x2
#define SND_NOSTOP 0x10
#define SND_NOWAIT 0x2000
#define SND_FILENAME 0x20000
#define SND_PURGE 0x40

#define HELP_CONTEXT 0x1
#define HELP_QUIT 0x2
#define HELP_INDEX 0x3
#define HELP_FINDER 0xB
#define HH_HELP_CONTEXT 0xF

#define ERROR_SUCCESS 0
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define HKEY_CURRENT_USER ((HKEY)0x80000001)
#define KEY_READ 0x20019
#define REG_SZ 1

#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_DELETE 0x2E
#define VK_NUMPAD0 0x60
#define VK_F1 0x70
#define FVIRTKEY 1
#define FSHIFT 0x04
#define FCONTROL 0x08
#define FALT 0x10

// messages
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_SIZE 0x0005
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ICONERASEBKGND 0x0027
#define WM_DRAWITEM 0x002B
#define WM_QUERYDRAGICON 0x0037
#define WM_NOTIFY 0x004E
#define WM_HELP 0x0053
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_USER 0x0400
#define SC_CLOSE 0xF060
#define SC_MINIMIZE 0xF020

#define BN_CLICKED 0
#define CN_COMMAND 0
#define EN_CHANGE 0x0300
#define EN_KILLFOCUS 0x0200
#define EN_SETFOCUS 0x0100
#define CBN_SELCHANGE 1
#define LBN_SELCHANGE 1
#define LBN_DBLCLK 2
#define NM_FIRST 0U
#define NM_CLICK (NM_FIRST - 2)
#define NM_DBLCLK (NM_FIRST - 3)
#define LVN_FIRST (0U - 100U)
#define LVN_ITEMCHANGING (LVN_FIRST - 0)
#define LVN_ITEMCHANGED (LVN_FIRST - 1)
#define LVN_COLUMNCLICK (LVN_FIRST - 8)
#define UDN_FIRST (0U - 721)
#define UDN_DELTAPOS (UDN_FIRST - 1)
#define TTN_FIRST (0U - 520U)
#define TTN_NEEDTEXT (TTN_FIRST - 0)
#define TTF_IDISHWND 0x1

// ---------------------------------------------------------------- structs
struct NMHDR { HWND hwndFrom; UINT_PTR idFrom; UINT code; };
typedef NMHDR *LPNMHDR;
struct NM_LISTVIEW { NMHDR hdr; int iItem; int iSubItem; UINT uNewState; UINT uOldState; UINT uChanged; POINT ptAction; LPARAM lParam; };
typedef NM_LISTVIEW NMLISTVIEW, *LPNMLISTVIEW;
struct NM_UPDOWN { NMHDR hdr; int iPos; int iDelta; };
typedef NM_UPDOWN NMUPDOWN;
struct TOOLTIPTEXT { NMHDR hdr; LPSTR lpszText; char szText[80]; HINSTANCE hinst; UINT uFlags; LPARAM lParam; };
struct HELPINFO { UINT cbSize; int iContextType; int iCtrlId; HANDLE hItemHandle; DWORD_PTR dwContextId; POINT MousePos; };
typedef HELPINFO *LPHELPINFO;
struct CREATESTRUCT { LPVOID lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent; int cy, cx, y, x; LONG style; LPCTSTR lpszName; LPCTSTR lpszClass; DWORD dwExStyle; };
typedef CREATESTRUCT *LPCREATESTRUCT;
struct DRAWITEMSTRUCT { UINT CtlType; UINT CtlID; UINT itemID; UINT itemAction; UINT itemState; HWND hwndItem; HDC hDC; RECT rcItem; UINT_PTR itemData; };
typedef DRAWITEMSTRUCT *LPDRAWITEMSTRUCT;
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; };
typedef MSG *LPMSG;
struct TEXTMETRIC { LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading, tmAveCharWidth, tmMaxCharWidth; };
struct BITMAP { LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; LPVOID bmBits; };
struct BITMAPV4HEADER { DWORD bV4Size; LONG bV4Width; LONG bV4Height; WORD bV4Planes; WORD bV4BitCount; DWORD rest[22]; };
struct BITMAPINFO;
struct PALETTEENTRY { BYTE peRed, peGreen, peBlue, peFlags; };
struct LOGPALETTE { WORD palVersion; WORD palNumEntries; PALETTEENTRY palPalEntry[1]; };
struct LVITEM { UINT mask; int iItem; int iSubItem; UINT state; UINT stateMask; LPSTR pszText; int cchTextMax; int iImage; LPARAM lParam; int iIndent; };
typedef LVITEM LV_ITEM;
struct LVCOLUMN { UINT mask; int fmt; int cx; LPSTR pszText; int cchTextMax; int iSubItem; };
typedef LVCOLUMN LV_COLUMN;
struct LVFINDINFO { UINT flags; LPCSTR psz; LPARAM lParam; POINT pt; UINT vkDirection; };
typedef LVFINDINFO LV_FINDINFO;
typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

// ---------------------------------------------------------------- CString
class CString {
public:
	std::string s;
	CString() {}
	CString(const char *p) : s(p ? (IS_INTRESOURCE(p) ? "" : p) : "") {}
	CString(const char *p, int n) : s(p, n) {}
	CString(char c, int n = 1) : s(n, c) {}
	CString(const CString &o) : s(o.s) {}
	CString(const std::string &o) : s(o) {}
	CString &operator=(const CString &o) { s = o.s; return *this; }
	CString &operator=(const char *p) { s = p ? p : ""; return *this; }
	CString &operator=(char c) { s = std::string(1, c); return *this; }
	operator const char *() const { return s.c_str(); }
	int GetLength() const { return (int)s.size(); }
	BOOL IsEmpty() const { return s.empty(); }
	void Empty() { s.clear(); }
	char GetAt(int i) const { return s[i]; }
	char operator[](int i) const { return s[i]; }
	void SetAt(int i, char c) { s[i] = c; }
	CString &operator+=(const char *p) { s += p; return *this; }
	CString &operator+=(const CString &o) { s += o.s; return *this; }
	CString &operator+=(char c) { s += c; return *this; }
	friend CString operator+(const CString &a, const CString &b) { return CString(a.s + b.s); }
	friend CString operator+(const CString &a, const char *b) { return CString(a.s + b); }
	friend CString operator+(const char *a, const CString &b) { return CString(a + b.s); }
	friend CString operator+(const CString &a, char b) { return CString(a.s + b); }
	friend bool operator==(const CString &a, const char *b) { return a.s == b; }
	friend bool operator==(const CString &a, const CString &b) { return a.s == b.s; }
	friend bool operator!=(const CString &a, const char *b) { return a.s != b; }
	friend bool operator!=(const CString &a, const CString &b) { return a.s != b.s; }
	friend bool operator<(const CString &a, const CString &b) { return a.s < b.s; }
	int Compare(const char *p) const { return strcmp(s.c_str(), p); }
	int CompareNoCase(const char *p) const { return strcasecmp(s.c_str(), p); }
	int Find(char c, int start = 0) const { size_t r = s.find(c, start); return r == std::string::npos ? -1 : (int)r; }
	int Find(const char *p, int start = 0) const { size_t r = s.find(p, start); return r == std::string::npos ? -1 : (int)r; }
	int ReverseFind(char c) const { size_t r = s.rfind(c); return r == std::string::npos ? -1 : (int)r; }
	CString Left(int n) const { return s.substr(0, max(0, min(n, (int)s.size()))); }
	CString Right(int n) const { n = max(0, min(n, (int)s.size())); return s.substr(s.size() - n); }
	CString Mid(int i) const { return i >= (int)s.size() ? CString() : CString(s.substr(i)); }
	CString Mid(int i, int n) const { return i >= (int)s.size() ? CString() : CString(s.substr(i, n)); }
	void MakeUpper() { for (auto &c : s) c = toupper((unsigned char)c); }
	void MakeLower() { for (auto &c : s) c = tolower((unsigned char)c); }
	void TrimLeft() { size_t i = s.find_first_not_of(" \t\r\n"); s = i == std::string::npos ? "" : s.substr(i); }
	void TrimRight() { size_t i = s.find_last_not_of(" \t\r\n"); s = i == std::string::npos ? "" : s.substr(0, i + 1); }
	int Replace(const char *from, const char *to);
	int Remove(char c);
	int Insert(int i, const char *p) { s.insert(min(i, (int)s.size()), p); return (int)s.size(); }
	int Delete(int i, int n = 1) { if (i < (int)s.size()) s.erase(i, n); return (int)s.size(); }
	char *GetBuffer(int n);
	char *GetBufferSetLength(int n) { return GetBuffer(n); }
	void ReleaseBuffer(int n = -1);
	void Format(const char *fmt, ...);
	void FormatV(const char *fmt, va_list ap);
	BOOL LoadString(UINT id);
private:
	std::vector<char> buf;
};
typedef const CString &LPCSTRREF;

// ---------------------------------------------------------------- CObject + collections
struct CRuntimeClass { const char *m_lpszClassName; };
class CArchive;
class CObject {
public:
	virtual ~CObject() {}
	virtual void Serialize(CArchive &) {}
	BOOL IsKindOf(const CRuntimeClass *) const { return TRUE; }
};
#define DECLARE_DYNAMIC(c)
#define DECLARE_DYNCREATE(c)
#define DECLARE_SERIAL(c)
#define IMPLEMENT_DYNAMIC(c, b)
#define IMPLEMENT_DYNCREATE(c, b)
#define IMPLEMENT_SERIAL(c, b, v)
#define RUNTIME_CLASS(c) ((CRuntimeClass *)0)

// doubly linked list of CObject*; POSITION is a node pointer
class CObList : public CObject {
	struct Node { Node *prev, *next; CObject *data; };
	Node *head = nullptr, *tail = nullptr;
	int count = 0;
	static Node *N(POSITION p) { return (Node *)p; }
public:
	~CObList() { RemoveAll(); }
	int GetCount() const { return count; }
	int GetSize() const { return count; }
	BOOL IsEmpty() const { return count == 0; }
	POSITION GetHeadPosition() const { return (POSITION)head; }
	POSITION GetTailPosition() const { return (POSITION)tail; }
	CObject *&GetHead() { return head->data; }
	CObject *&GetTail() { return tail->data; }
	CObject *GetHead() const { return head->data; }
	CObject *GetTail() const { return tail->data; }
	CObject *&GetNext(POSITION &p) { Node *n = N(p); p = (POSITION)n->next; return n->data; }
	CObject *&GetPrev(POSITION &p) { Node *n = N(p); p = (POSITION)n->prev; return n->data; }
	CObject *&GetAt(POSITION p) { return N(p)->data; }
	void SetAt(POSITION p, CObject *o) { N(p)->data = o; }
	POSITION AddHead(CObject *o) { return InsertBefore((POSITION)head, o); }
	POSITION AddTail(CObject *o) { return InsertAfter((POSITION)tail, o); }
	void AddTail(CObList *l) { for (POSITION p = l->GetHeadPosition(); p;) AddTail(l->GetNext(p)); }
	void AddHead(CObList *l) { for (POSITION p = l->GetTailPosition(); p;) AddHead(l->GetPrev(p)); }
	POSITION InsertBefore(POSITION p, CObject *o);
	POSITION InsertAfter(POSITION p, CObject *o);
	CObject *RemoveHead() { CObject *o = head->data; RemoveAt((POSITION)head); return o; }
	CObject *RemoveTail() { CObject *o = tail->data; RemoveAt((POSITION)tail); return o; }
	void RemoveAt(POSITION p);
	void RemoveAll() { while (head) RemoveAt((POSITION)head); }
	POSITION Find(CObject *o, POSITION after = NULL) const;
	POSITION FindIndex(int i) const;
};
class CPtrList : public CObject {
	CObList l;
public:
	int GetCount() const { return l.GetCount(); }
	BOOL IsEmpty() const { return l.IsEmpty(); }
	POSITION GetHeadPosition() const { return l.GetHeadPosition(); }
	void *GetNext(POSITION &p) { return (void *)l.GetNext(p); }
	POSITION AddTail(void *p) { return l.AddTail((CObject *)p); }
	void *RemoveHead() { return (void *)l.RemoveHead(); }
	void RemoveAll() { l.RemoveAll(); }
};
class CStringArray : public CObject {
	std::vector<CString> v;
public:
	int GetSize() const { return (int)v.size(); }
	int GetCount() const { return (int)v.size(); }
	int GetUpperBound() const { return (int)v.size() - 1; }
	void SetSize(int n, int = -1) { v.resize(n); }
	const CString &GetAt(int i) const { return v[i]; }
	void SetAt(int i, const char *p) { v[i] = p; }
	void SetAtGrow(int i, const char *p) { if (i >= (int)v.size()) v.resize(i + 1); v[i] = p; }
	int Add(const char *p) { v.push_back(p); return (int)v.size() - 1; }
	CString &operator[](int i) { return v[i]; }
	void RemoveAll() { v.clear(); }
	void RemoveAt(int i, int n = 1) { v.erase(v.begin() + i, v.begin() + i + n); }
};
class CDWordArray : public CObject {
	std::vector<DWORD> v;
public:
	int GetSize() const { return (int)v.size(); }
	void SetSize(int n, int = -1) { v.resize(n); }
	DWORD GetAt(int i) const { return v[i]; }
	void SetAt(int i, DWORD d) { v[i] = d; }
	int Add(DWORD d) { v.push_back(d); return (int)v.size() - 1; }
	DWORD &operator[](int i) { return v[i]; }
	void RemoveAll() { v.clear(); }
};

// ---------------------------------------------------------------- files
class CException {
public:
	virtual ~CException() {}
	void Delete() { delete this; }
};
class CArchiveException : public CException {
public:
	enum { none, generic, readOnly, endOfFile, writeOnly, badIndex, badClass, badSchema };
	int m_cause;
	CArchiveException(int c = none) : m_cause(c) {}
};
class CFileException : public CException {
public:
	int m_cause = 0;
};
class CFile : public CObject {
public:
	enum { modeRead = 0, modeWrite = 1, modeReadWrite = 2, shareExclusive = 0x10, shareDenyWrite = 0x20, shareDenyNone = 0x40, modeCreate = 0x1000, modeNoTruncate = 0x2000, typeBinary = 0x8000 };
	enum { begin = 0, current = 1, end = 2 };
	FILE *m_fp = nullptr;
	BOOL m_bWrite = FALSE;
	CString m_strFileName;
	~CFile() { Close(); }
	virtual BOOL Open(LPCTSTR name, UINT flags, CFileException * = NULL);
	virtual UINT Read(void *buf, UINT n);
	virtual void Write(const void *buf, UINT n);
	virtual long Seek(long off, UINT from);
	virtual void Close();
	CString GetFilePath() const { return m_strFileName; }
};
class CArchive {
public:
	enum Mode { store = 0, load = 1 };
	CArchive(CFile *f, UINT mode, int = 4096, void * = NULL) : m_pFile(f), m_nMode(mode) {}
	BOOL IsLoading() const { return m_nMode == load; }
	BOOL IsStoring() const { return m_nMode == store; }
	void Close() {}
	void Flush() {}
	UINT Read(void *p, UINT n);
	void Write(const void *p, UINT n);
	CFile *GetFile() { return m_pFile; }
#define AR_IO(T) CArchive &operator<<(T v) { Write(&v, sizeof(v)); return *this; } \
	CArchive &operator>>(T &v) { if (Read(&v, sizeof(v)) != sizeof(v)) throw new CArchiveException(CArchiveException::endOfFile); return *this; }
	AR_IO(BYTE) AR_IO(char) AR_IO(WORD) AR_IO(short) AR_IO(int) AR_IO(UINT) AR_IO(float) AR_IO(double)
#undef AR_IO
	CArchive &operator<<(const CString &s);
	CArchive &operator>>(CString &s);
	CArchive &operator<<(const char *s) { return *this << CString(s); }
	CArchive &operator<<(POINT p) { return *this << (int)p.x << (int)p.y; }
	CArchive &operator>>(POINT &p) { int x, y; *this >> x >> y; p.x = x; p.y = y; return *this; }
	CArchive &operator<<(SIZE p) { return *this << (int)p.cx << (int)p.cy; }
	CArchive &operator>>(SIZE &p) { int x, y; *this >> x >> y; p.cx = x; p.cy = y; return *this; }
	CFile *m_pFile;
	UINT m_nMode;
};

// ---------------------------------------------------------------- GDI
struct Bitmap {
	int w = 0, h = 0;
	std::vector<DWORD> px; // COLORREF values (0x00BBGGRR)
	Bitmap(int ww = 0, int hh = 0, DWORD fill = 0) : w(ww), h(hh), px((size_t)ww * hh, fill) {}
	DWORD *row(int y) { return &px[(size_t)y * w]; }
};
class CGdiObject : public CObject {
public:
	HGDIOBJ m_hObject = nullptr;
	operator HGDIOBJ() const { return m_hObject; }
	HGDIOBJ GetSafeHandle() const { return m_hObject; }
	BOOL DeleteObject() { m_hObject = nullptr; return TRUE; }
};
class CBitmap : public CGdiObject {
public:
	operator HBITMAP() const { return (HBITMAP)m_hObject; }
	BOOL LoadBitmap(UINT id);
	BOOL LoadBitmap(LPCTSTR name);
	BOOL LoadOEMBitmap(UINT id);
	BOOL Attach(HGDIOBJ h) { m_hObject = h; return TRUE; }
	HGDIOBJ Detach() { HGDIOBJ h = m_hObject; m_hObject = nullptr; return h; }
	int GetBitmap(BITMAP *bm);
	BOOL CreateCompatibleBitmap(CDC *, int w, int h) { m_hObject = new Bitmap(w, h); return TRUE; }
	static CBitmap *FromHandle(HBITMAP h);
};
class CPen : public CGdiObject {
public:
	COLORREF m_cr = 0;
	CPen() {}
	CPen(int, int, COLORREF c) { m_cr = c; m_hObject = this; }
	BOOL CreatePen(int, int, COLORREF c) { m_cr = c; m_hObject = this; return TRUE; }
};
class CBrush : public CGdiObject {
public:
	COLORREF m_cr = 0;
	CBrush() {}
	CBrush(COLORREF c) { m_cr = c; m_hObject = this; }
	BOOL CreateSolidBrush(COLORREF c) { m_cr = c; m_hObject = this; return TRUE; }
};
class CFont : public CGdiObject {
public:
	int m_bold = 0;
};
class CPalette : public CGdiObject {};
class CRgn : public CGdiObject {};

class CDC : public CObject {
public:
	HDC m_hDC;
	Bitmap *m_surf = nullptr;   // target pixels
	int m_ox = 0, m_oy = 0;     // device origin (logical 0,0) in target
	CRect m_clip;               // clip in target coordinates
	CWnd *m_pWnd = nullptr;     // window DCs
	HBITMAP m_selBitmap = nullptr;
	COLORREF m_crText = 0, m_crBk = 0xFFFFFF;
	int m_nBkMode = OPAQUE;
	int m_font = 1;             // 0 normal (dialog), 1 bold (system)
	COLORREF m_crPen = 0;
	CPoint m_pos;
	CDC() { m_hDC = this; }
	virtual ~CDC() {}
	HDC GetSafeHdc() const { return m_hDC; }
	static CDC *FromHandle(HDC h) { return h; }
	BOOL CreateCompatibleDC(CDC *) { return TRUE; }
	BOOL DeleteDC() { return TRUE; }
	HBITMAP SelectObject(HBITMAP h);
	CBitmap *SelectObject(CBitmap *b);
	CPen *SelectObject(CPen *p) { m_crPen = p ? p->m_cr : 0; return nullptr; }
	CBrush *SelectObject(CBrush *) { return nullptr; }
	CFont *SelectObject(CFont *f) { if (f) m_font = f->m_bold; return nullptr; }
	HGDIOBJ SelectStockObject(int i);
	int SetMapMode(int) { return MM_TEXT; }
	COLORREF SetTextColor(COLORREF c) { COLORREF o = m_crText; m_crText = c; return o; }
	COLORREF SetBkColor(COLORREF c) { COLORREF o = m_crBk; m_crBk = c; return o; }
	COLORREF GetTextColor() const { return m_crText; }
	COLORREF GetBkColor() const { return m_crBk; }
	int SetBkMode(int m) { int o = m_nBkMode; m_nBkMode = m; return o; }
	void FillSolidRect(int x, int y, int cx, int cy, COLORREF c);
	void FillSolidRect(LPCRECT r, COLORREF c) { FillSolidRect(r->left, r->top, r->right - r->left, r->bottom - r->top, c); }
	void FillRect(LPCRECT r, CBrush *b) { FillSolidRect(r, b ? b->m_cr : 0); }
	void FrameRect(LPCRECT r, CBrush *b);
	void Draw3dRect(int x, int y, int cx, int cy, COLORREF tl, COLORREF br);
	void Draw3dRect(LPCRECT r, COLORREF tl, COLORREF br) { Draw3dRect(r->left, r->top, r->right - r->left, r->bottom - r->top, tl, br); }
	BOOL Rectangle(int l, int t, int r, int b);
	BOOL BitBlt(int x, int y, int cx, int cy, CDC *src, int sx, int sy, DWORD rop);
	BOOL StretchBlt(int x, int y, int cx, int cy, CDC *src, int sx, int sy, int scx, int scy, DWORD rop);
	COLORREF SetPixel(int x, int y, COLORREF c);
	COLORREF GetPixel(int x, int y) const;
	CPoint MoveTo(int x, int y) { CPoint o = m_pos; m_pos = CPoint(x, y); return o; }
	CPoint MoveTo(POINT p) { return MoveTo(p.x, p.y); }
	BOOL LineTo(int x, int y);
	BOOL LineTo(POINT p) { return LineTo(p.x, p.y); }
	BOOL TextOut(int x, int y, const char *s, int n);
	BOOL TextOut(int x, int y, const CString &s) { return TextOut(x, y, s, s.GetLength()); }
	BOOL ExtTextOut(int x, int y, UINT opt, LPCRECT r, const CString &s, int *dx);
	int DrawText(const CString &s, LPRECT r, UINT fmt);
	CSize GetTextExtent(const char *s, int n) const;
	CSize GetTextExtent(const CString &s) const { return GetTextExtent(s, s.GetLength()); }
	BOOL GetTextMetrics(TEXTMETRIC *tm) const;
	int GetClipBox(LPRECT r) const;
	BOOL DrawState(CPoint pt, CSize sz, HBITMAP h, UINT flags, CBrush * = NULL);
	BOOL DrawIcon(int, int, HICON) { return TRUE; }
	void blit(Bitmap *src, int sx, int sy, int x, int y, int cx, int cy, DWORD rop, bool keyed = false, DWORD key = 0);
	void hline(int x0, int x1, int y, COLORREF c);
	void vline(int x, int y0, int y1, COLORREF c);
};
class CPaintDC : public CDC {
public:
	CPaintDC(CWnd *w);
};
class CClientDC : public CDC {
public:
	CClientDC(CWnd *w);
};
class CWindowDC : public CDC {
public:
	CWindowDC(CWnd *w);
};

class CImageList : public CObject {
public:
	std::vector<Bitmap *> m_imgs;  // one per image
	int m_cx = 0, m_cy = 0;
	std::vector<DWORD> m_mask;     // mask colour per image, CLR_NONE for none
	BOOL Create(UINT idBitmap, int cx, int grow, COLORREF mask);
	BOOL Create(int cx, int cy, UINT flags, int initial, int grow) { m_cx = cx; m_cy = cy; return TRUE; }
	int Add(CBitmap *b, COLORREF mask);
	int GetImageCount() const { return (int)m_imgs.size(); }
	BOOL Draw(CDC *dc, int i, POINT pt, UINT style);
	BOOL DrawIndirect(CDC *dc, int i, POINT pt, SIZE sz, POINT org, UINT style = ILD_NORMAL, DWORD rop = SRCCOPY, COLORREF bk = CLR_DEFAULT, COLORREF fg = CLR_DEFAULT);
	BOOL DeleteImageList() { return TRUE; }
	HICON ExtractIcon(int) { return nullptr; }
};

// ---------------------------------------------------------------- message maps
class CCmdTarget;
typedef void (CCmdTarget::*AFX_PMSG)(void);
enum AfxSig { AfxSig_end = 0, AfxSig_vv, AfxSig_is, AfxSig_bHELPINFO, AfxSig_vNMHDRpl, AfxSig_bNMHDRpl, AfxSig_vOWNER, AfxSig_vwp, AfxSig_vwl, AfxSig_hv, AfxSig_lwl, AfxSig_bv };
struct AFX_MSGMAP_ENTRY { UINT nMessage; UINT nCode; UINT nID; UINT nLastID; int nSig; AFX_PMSG pfn; };
struct AFX_MSGMAP { const AFX_MSGMAP *(*pfnGetBaseMap)(); const AFX_MSGMAP_ENTRY *lpEntries; };
struct CREATESTRUCT;

// build entries from a member pointer; the signature is taken from its type
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, void (T::*f)()) { return {m, c, id, id, AfxSig_vv, (AFX_PMSG)f}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, BOOL (T::*f)()) { return {m, c, id, id, AfxSig_bv, reinterpret_cast<AFX_PMSG>(static_cast<BOOL (CCmdTarget::*)()>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, int (T::*f)(LPCREATESTRUCT)) { return {m, c, id, id, AfxSig_is, reinterpret_cast<AFX_PMSG>(static_cast<int (CCmdTarget::*)(LPCREATESTRUCT)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, BOOL (T::*f)(HELPINFO *)) { return {m, c, id, id, AfxSig_bHELPINFO, reinterpret_cast<AFX_PMSG>(static_cast<BOOL (CCmdTarget::*)(HELPINFO *)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, void (T::*f)(NMHDR *, LRESULT *)) { return {m, c, id, id, AfxSig_vNMHDRpl, reinterpret_cast<AFX_PMSG>(static_cast<void (CCmdTarget::*)(NMHDR *, LRESULT *)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, BOOL (T::*f)(UINT, NMHDR *, LRESULT *)) { return {m, c, id, id, AfxSig_bNMHDRpl, reinterpret_cast<AFX_PMSG>(static_cast<BOOL (CCmdTarget::*)(UINT, NMHDR *, LRESULT *)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, void (T::*f)(int, LPDRAWITEMSTRUCT)) { return {m, c, id, id, AfxSig_vOWNER, reinterpret_cast<AFX_PMSG>(static_cast<void (CCmdTarget::*)(int, LPDRAWITEMSTRUCT)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, void (T::*f)(UINT, CPoint)) { return {m, c, id, id, AfxSig_vwp, reinterpret_cast<AFX_PMSG>(static_cast<void (CCmdTarget::*)(UINT, CPoint)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, void (T::*f)(UINT, LPARAM)) { return {m, c, id, id, AfxSig_vwl, reinterpret_cast<AFX_PMSG>(static_cast<void (CCmdTarget::*)(UINT, LPARAM)>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, HCURSOR (T::*f)()) { return {m, c, id, id, AfxSig_hv, reinterpret_cast<AFX_PMSG>(static_cast<HCURSOR (CCmdTarget::*)()>(f))}; }
template <class T> AFX_MSGMAP_ENTRY afxE(UINT m, UINT c, UINT id, LRESULT (T::*f)(WPARAM, LPARAM)) { return {m, c, id, id, AfxSig_lwl, reinterpret_cast<AFX_PMSG>(static_cast<LRESULT (CCmdTarget::*)(WPARAM, LPARAM)>(f))}; }

#define DECLARE_MESSAGE_MAP() \
protected: \
	static const AFX_MSGMAP *GetThisMessageMap(); \
	virtual const AFX_MSGMAP *GetMessageMap() const; \
public:
#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	const AFX_MSGMAP *theClass::GetMessageMap() const { return GetThisMessageMap(); } \
	const AFX_MSGMAP *theClass::GetThisMessageMap() { \
		typedef theClass ThisClass; \
		typedef baseClass TheBaseClass; \
		(void)sizeof(ThisClass); \
		static const AFX_MSGMAP_ENTRY _messageEntries[] = {
#define END_MESSAGE_MAP() \
		{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0}}; \
		static const AFX_MSGMAP messageMap = {&TheBaseClass::GetThisMessageMap, &_messageEntries[0]}; \
		return &messageMap; }

#define ON_WM_PAINT() afxE<ThisClass>(WM_PAINT, 0, 0, &ThisClass::OnPaint),
#define ON_WM_CREATE() afxE<ThisClass>(WM_CREATE, 0, 0, &ThisClass::OnCreate),
#define ON_WM_DESTROY() afxE<ThisClass>(WM_DESTROY, 0, 0, &ThisClass::OnDestroy),
#define ON_WM_CLOSE() afxE<ThisClass>(WM_CLOSE, 0, 0, &ThisClass::OnClose),
#define ON_WM_HELPINFO() afxE<ThisClass>(WM_HELP, 0, 0, &ThisClass::OnHelpInfo),
#define ON_WM_DRAWITEM() afxE<ThisClass>(WM_DRAWITEM, 0, 0, &ThisClass::OnDrawItem),
#define ON_WM_LBUTTONUP() afxE<ThisClass>(WM_LBUTTONUP, 0, 0, &ThisClass::OnLButtonUp),
#define ON_WM_LBUTTONDOWN() afxE<ThisClass>(WM_LBUTTONDOWN, 0, 0, &ThisClass::OnLButtonDown),
#define ON_WM_RBUTTONUP() afxE<ThisClass>(WM_RBUTTONUP, 0, 0, &ThisClass::OnRButtonUp),
#define ON_WM_SYSCOMMAND() afxE<ThisClass>(WM_SYSCOMMAND, 0, 0, &ThisClass::OnSysCommand),
#define ON_WM_QUERYDRAGICON() afxE<ThisClass>(WM_QUERYDRAGICON, 0, 0, &ThisClass::OnQueryDragIcon),
#define ON_COMMAND(id, fn) afxE<ThisClass>(WM_COMMAND, CN_COMMAND, id, &ThisClass::fn),
#define ON_BN_CLICKED(id, fn) afxE<ThisClass>(WM_COMMAND, BN_CLICKED, id, &ThisClass::fn),
#define ON_CBN_SELCHANGE(id, fn) afxE<ThisClass>(WM_COMMAND, CBN_SELCHANGE, id, &ThisClass::fn),
#define ON_LBN_SELCHANGE(id, fn) afxE<ThisClass>(WM_COMMAND, LBN_SELCHANGE, id, &ThisClass::fn),
#define ON_EN_KILLFOCUS(id, fn) afxE<ThisClass>(WM_COMMAND, EN_KILLFOCUS, id, &ThisClass::fn),
#define ON_EN_CHANGE(id, fn) afxE<ThisClass>(WM_COMMAND, EN_CHANGE, id, &ThisClass::fn),
#define ON_NOTIFY(code, id, fn) afxE<ThisClass>(WM_NOTIFY, (UINT)(code), id, &ThisClass::fn),
#define ON_NOTIFY_EX(code, id, fn) afxE<ThisClass>(WM_NOTIFY, (UINT)(code), 0xFFFFFFFF, &ThisClass::fn),
#define ON_MESSAGE(msg, fn) afxE<ThisClass>(msg, 0, 0, &ThisClass::fn),

// ---------------------------------------------------------------- windows
class CCmdTarget : public CObject {
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void *pExtra);
	const AFX_MSGMAP_ENTRY *FindEntry(UINT msg, UINT code, UINT id) const;
protected:
	static const AFX_MSGMAP *GetThisMessageMap();
	virtual const AFX_MSGMAP *GetMessageMap() const;
	friend class CWnd;
};

class CMenu;
class CScrollBarInfo;
class CWnd : public CCmdTarget {
	DECLARE_MESSAGE_MAP()
public:
	HWND m_hWnd = nullptr;
	CWnd *m_parent = nullptr;
	std::vector<CWnd *> m_children;
	CRect m_rect;                 // window rect in parent client (top-level: screen)
	int m_clientX = 0, m_clientY = 0; // client origin inside the window rect
	DWORD m_style = 0, m_exStyle = 0;
	UINT m_nID = 0;
	CString m_text;
	BOOL m_bDirty = TRUE;
	BOOL m_bOwned = FALSE;        // shim created it (dialog controls): delete on destroy
	Bitmap *m_backing = nullptr;  // top-level windows only
	int m_font = 1;               // 0 dialog font, 1 system font
	BOOL m_bToolTips = FALSE;

	CWnd();
	virtual ~CWnd();
	operator HWND() const { return m_hWnd; }
	HWND GetSafeHwnd() const { return this ? m_hWnd : nullptr; }
	static CWnd *FromHandle(HWND h) { return h; }
	static CWnd *FromHandlePermanent(HWND h) { return h; }

	virtual BOOL Create(LPCTSTR cls, LPCTSTR name, DWORD style, const RECT &r, CWnd *parent, UINT id, void * = NULL);
	virtual BOOL CreateEx(DWORD exStyle, LPCTSTR cls, LPCTSTR name, DWORD style, int x, int y, int cx, int cy, HWND parent, HMENU id, LPVOID = NULL);
	BOOL CreateEx(DWORD exStyle, LPCTSTR cls, LPCTSTR name, DWORD style, const RECT &r, CWnd *parent, UINT id, LPVOID = NULL) { return CreateEx(exStyle, cls, name, style, r.left, r.top, r.right - r.left, r.bottom - r.top, parent, (HMENU)(uintptr_t)id); }
	virtual BOOL PreCreateWindow(CREATESTRUCT &cs) { return TRUE; }
	virtual BOOL DestroyWindow();
	virtual void PostNcDestroy() {}
	virtual BOOL PreTranslateMessage(MSG *pMsg) { return FALSE; }
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
	virtual BOOL OnCommand(WPARAM wp, LPARAM lp);
	virtual BOOL OnNotify(WPARAM wp, LPARAM lp, LRESULT *res);
	virtual void DrawItem(LPDRAWITEMSTRUCT) {}
	virtual BOOL IsDialog() const { return FALSE; }

	// shim hooks
	virtual void ShimPaint(CDC &dc) {}         // default look of a control
	virtual BOOL ShimMouse(UINT msg, CPoint pt) { return FALSE; }
	virtual BOOL ShimKey(UINT vk, BOOL down) { return FALSE; }
	virtual BOOL ShimChar(UINT ch) { return FALSE; }
	virtual BOOL ShimWheel(int dy) { return FALSE; }
	virtual BOOL ShimWantsFocus() const { return FALSE; }
	virtual void ShimAdopt(CWnd *generic) {}     // DDX_Control: take over a created control
	BOOL HasHandler(UINT msg) const;

	// messages we route
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT cs) { return 0; }
	afx_msg void OnDestroy() {}
	afx_msg void OnClose() { DestroyWindow(); }
	afx_msg BOOL OnHelpInfo(HELPINFO *) { return FALSE; }
	afx_msg void OnDrawItem(int id, LPDRAWITEMSTRUCT dis);
	afx_msg void OnLButtonUp(UINT, CPoint) {}
	afx_msg void OnLButtonDown(UINT, CPoint) {}
	afx_msg void OnRButtonUp(UINT, CPoint) {}
	afx_msg void OnSysCommand(UINT id, LPARAM);

	// API
	CWnd *GetParent() const { return m_parent; }
	CWnd *GetTopLevel();
	CWnd *GetDlgItem(int id) const;
	int GetDlgCtrlID() const { return m_nID; }
	void SetDlgCtrlID(int id) { m_nID = id; }
	void GetClientRect(LPRECT r) const { r->left = r->top = 0; r->right = m_rect.Width() - m_clientX - m_clientRight; r->bottom = m_rect.Height() - m_clientY - m_clientBottom; }
	void GetWindowRect(LPRECT r) const;
	void ClientToScreen(LPPOINT p) const;
	void ClientToScreen(LPRECT r) const;
	void ScreenToClient(LPPOINT p) const;
	void ScreenToClient(LPRECT r) const;
	void MapWindowPoints(CWnd *to, LPPOINT p, UINT n) const;
	void MapWindowPoints(CWnd *to, LPRECT r) const;
	BOOL ShowWindow(int cmd);
	BOOL IsWindowVisible() const;
	BOOL m_bVisible = FALSE;
	BOOL EnableWindow(BOOL e = TRUE);
	BOOL IsWindowEnabled() const { return !(m_style & WS_DISABLED); }
	BOOL IsWindow() const { return m_hWnd != nullptr; }
	BOOL IsIconic() const { return FALSE; }
	void SetWindowText(LPCTSTR s);
	void GetWindowText(CString &s) const { s = m_text; }
	int GetWindowText(LPTSTR buf, int n) const { strncpy(buf, m_text, n); if (n) buf[n - 1] = 0; return (int)strlen(buf); }
	int GetWindowTextLength() const { return m_text.GetLength(); }
	void SetDlgItemText(int id, LPCTSTR s) { if (CWnd *w = GetDlgItem(id)) w->SetWindowText(s); }
	int GetDlgItemText(int id, CString &s) const { CWnd *w = GetDlgItem(id); s = w ? w->m_text : CString(); return s.GetLength(); }
	void SetDlgItemInt(int id, UINT v, BOOL sig = TRUE) { CString s; s.Format(sig ? "%d" : "%u", v); SetDlgItemText(id, s); }
	UINT GetDlgItemInt(int id, BOOL *ok = NULL, BOOL = TRUE) const { CString s; GetDlgItemText(id, s); if (ok) *ok = TRUE; return atoi(s); }
	void CheckDlgButton(int id, UINT c);
	UINT IsDlgButtonChecked(int id) const;
	void CheckRadioButton(int first, int last, int check);
	void Invalidate(BOOL = TRUE);
	void InvalidateRect(LPCRECT, BOOL = TRUE) { Invalidate(); }
	BOOL RedrawWindow(LPCRECT = NULL, CRgn * = NULL, UINT = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	void UpdateWindow();
	BOOL SetWindowPos(const CWnd *after, int x, int y, int cx, int cy, UINT flags);
	void MoveWindow(int x, int y, int cx, int cy, BOOL = TRUE);
	void MoveWindow(LPCRECT r, BOOL b = TRUE) { MoveWindow(r->left, r->top, r->right - r->left, r->bottom - r->top, b); }
	void CenterWindow(CWnd * = NULL);
	CWnd *SetFocus();
	static CWnd *GetFocus();
	static CWnd *GetActiveWindow();
	CWnd *SetActiveWindow() { return this; }
	void SetForegroundWindow() {}
	void BringWindowToTop() {}
	HICON SetIcon(HICON, BOOL) { return nullptr; }
	CDC *GetDC();
	CDC *GetWindowDC() { return GetDC(); }
	int ReleaseDC(CDC *dc);
	LRESULT SendMessage(UINT msg, WPARAM wp = 0, LPARAM lp = 0);
	BOOL PostMessage(UINT msg, WPARAM wp = 0, LPARAM lp = 0);
	BOOL EnableToolTips(BOOL b = TRUE) { m_bToolTips = b; return TRUE; }
	void WinHelp(DWORD data, UINT cmd = HELP_CONTEXT);
	CFont *GetFont() const { return nullptr; }
	void SetFont(CFont *f, BOOL = TRUE) { if (f) m_font = f->m_bold; }
	LONG GetStyle() const { return m_style; }
	BOOL ModifyStyle(DWORD rem, DWORD add, UINT = 0) { m_style = (m_style & ~rem) | add; Invalidate(); return TRUE; }
	int MessageBox(LPCTSTR text, LPCTSTR caption = NULL, UINT type = MB_OK);
	CMenu *GetSystemMenu(BOOL) { return nullptr; }
	BOOL SubclassDlgItem(UINT id, CWnd *parent);
	int m_clientRight = 0, m_clientBottom = 0;
	// geometry helpers (shim)
	CPoint ScreenOrigin() const;   // client origin in screen coords
	CRect VisibleRect() const;     // visible part in top-level backing coords
	CPoint BackingOrigin() const;  // client origin in top-level backing coords
};

class CDataExchange {
public:
	BOOL m_bSaveAndValidate;
	BOOL m_bDiscover = FALSE; // shim: collect DDX_Control before controls exist
	CWnd *m_pDlgWnd;
	CDataExchange(CWnd *w, BOOL save) : m_bSaveAndValidate(save), m_pDlgWnd(w) {}
	void Fail();
	struct Discovered { int id; CWnd *w; };
	std::vector<Discovered> m_found;
};

struct DlgTemplate;
class CDialog : public CWnd {
	DECLARE_MESSAGE_MAP()
public:
	UINT m_nIDTemplate = 0;
	CWnd *m_pParentWnd = nullptr;
	int m_nModalResult = -1;
	BOOL m_bModalDone = FALSE;
	const DlgTemplate *m_pTemplate = nullptr; // or a runtime one
	CDialog() {}
	CDialog(UINT id, CWnd *parent = NULL) : m_nIDTemplate(id), m_pParentWnd(parent) {}
	virtual INT_PTR DoModal();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange *) {}
	virtual BOOL IsDialog() const { return TRUE; }
	void EndDialog(int r);
	BOOL UpdateData(BOOL save = TRUE);
	void NextDlgCtrl();
	void PrevDlgCtrl();
	void GotoDlgCtrl(CWnd *w) { if (w) w->SetFocus(); }
	void MapDialogRect(LPRECT r) const;
	BOOL CreateFromTemplate(const DlgTemplate *t, CWnd *parent);
	CWnd *DefaultButton();
	afx_msg void OnPaint();
};

class CButton : public CWnd {
public:
	int m_check = 0;
	BOOL m_pressed = FALSE, m_hot = FALSE;
	BOOL Create(LPCTSTR text, DWORD style, const RECT &r, CWnd *parent, UINT id);
	int GetCheck() const { return m_check; }
	void SetCheck(int c) { m_check = c; Invalidate(); }
	UINT GetState() const { return (m_check ? 3 : 0) | (m_pressed ? 4 : 0); }
	void SetState(BOOL b) { m_pressed = b; Invalidate(); }
	UINT GetButtonStyle() const { return m_style & 0xFF; }
	void SetButtonStyle(UINT s, BOOL = TRUE) { m_style = (m_style & ~0xFF) | s; Invalidate(); }
	HBITMAP SetBitmap(HBITMAP h) { HBITMAP o = m_bmp; m_bmp = h; Invalidate(); return o; }
	HBITMAP m_bmp = nullptr;
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimKey(UINT vk, BOOL down) override;
	BOOL ShimWantsFocus() const override;
	void ShimAdopt(CWnd *g) override { m_check = ((CButton *)g)->m_check; }
	void Click();
	int Type() const { return m_style & BS_TYPEMASK; }
};
class CBitmapButton : public CButton {
public:
	CBitmap m_bitmap, m_bitmapSel, m_bitmapFocus, m_bitmapDisabled;
	BOOL LoadBitmaps(UINT a, UINT b = 0, UINT c = 0, UINT d = 0);
	void SizeToContent();
	void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;
};
class CStatic : public CWnd {
public:
	HBITMAP m_bmp = nullptr;
	BOOL Create(LPCTSTR text, DWORD style, const RECT &r, CWnd *parent, UINT id = 0xffff);
	HBITMAP SetBitmap(HBITMAP h) { HBITMAP o = m_bmp; m_bmp = h; Invalidate(); return o; }
	HBITMAP GetBitmap() const { return m_bmp; }
	void ShimPaint(CDC &dc) override;
	void ShimAdopt(CWnd *g) override { m_bmp = ((CStatic *)g)->m_bmp; }
};
class CEdit : public CWnd {
public:
	int m_caret = 0;
	int m_limit = 0;
	BOOL m_sel = FALSE; // whole text selected (after focus)
	void SetLimitText(UINT n) { m_limit = n; }
	void LimitText(int n = 0) { m_limit = n; }
	void SetSel(int, int, BOOL = FALSE) { m_sel = TRUE; m_caret = m_text.GetLength(); Invalidate(); }
	void SetReadOnly(BOOL b = TRUE) { if (b) m_style |= ES_READONLY; else m_style &= ~ES_READONLY; Invalidate(); }
	void ReplaceSel(LPCTSTR s, BOOL = FALSE);
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimKey(UINT vk, BOOL down) override;
	BOOL ShimChar(UINT ch) override;
	BOOL ShimWantsFocus() const override { return !(m_style & ES_READONLY) && IsWindowEnabled(); }
};
class CListBox : public CWnd {
public:
	struct Item { CString text; DWORD_PTR data; };
	std::vector<Item> m_items;
	int m_sel = -1, m_top = 0;
	int AddString(LPCTSTR s);
	int InsertString(int i, LPCTSTR s);
	int DeleteString(UINT i);
	void ResetContent() { m_items.clear(); m_sel = -1; m_top = 0; Invalidate(); }
	int GetCount() const { return (int)m_items.size(); }
	int GetCurSel() const { return m_sel; }
	int SetCurSel(int i);
	int GetText(int i, CString &s) const { if (i < 0 || i >= GetCount()) return LB_ERR; s = m_items[i].text; return s.GetLength(); }
	int GetText(int i, LPTSTR buf) const { if (i < 0 || i >= GetCount()) return LB_ERR; strcpy(buf, m_items[i].text); return (int)strlen(buf); }
	int GetTextLen(int i) const { return (i < 0 || i >= GetCount()) ? LB_ERR : m_items[i].text.GetLength(); }
	DWORD_PTR GetItemData(int i) const { return (i < 0 || i >= GetCount()) ? 0 : m_items[i].data; }
	int SetItemData(int i, DWORD_PTR d) { if (i < 0 || i >= GetCount()) return LB_ERR; m_items[i].data = d; return 0; }
	int FindStringExact(int start, LPCTSTR s) const;
	int VisibleRows() const;
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimKey(UINT vk, BOOL down) override;
	BOOL ShimWheel(int dy) override;
	BOOL ShimWantsFocus() const override { return IsWindowEnabled(); }
	void ShimAdopt(CWnd *g) override { m_items = ((CListBox *)g)->m_items; }
	void Notify(UINT code);
};
class CComboBox : public CListBox {
public:
	BOOL m_dropped = FALSE;
	int GetLBText(int i, LPTSTR buf) const { return GetText(i, buf); }
	void GetLBText(int i, CString &s) const { GetText(i, s); }
	int GetLBTextLen(int i) const { return GetTextLen(i); }
	int SelectString(int start, LPCTSTR s);
	int FindString(int start, LPCTSTR s) const;
	void ShowDropDown(BOOL b = TRUE) { m_dropped = b; }
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimKey(UINT vk, BOOL down) override;
	BOOL ShimWheel(int dy) override;
};
class CSpinButtonCtrl : public CWnd {
public:
	int m_lo = 0, m_hi = 100, m_pos = 0;
	int m_pressed = 0;
	void SetRange(int lo, int hi) { m_lo = lo; m_hi = hi; }
	void SetRange32(int lo, int hi) { SetRange(lo, hi); }
	void GetRange(int &lo, int &hi) const { lo = m_lo; hi = m_hi; }
	int SetPos(int p) { int o = m_pos; m_pos = p; return o; }
	int GetPos() const { return m_pos; }
	CWnd *SetBuddy(CWnd *w) { return w; }
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
};
class CProgressCtrl : public CWnd {
public:
	int m_lo = 0, m_hi = 100, m_pos = 0;
	void SetRange(short lo, short hi) { m_lo = lo; m_hi = hi; Invalidate(); }
	int SetPos(int p) { int o = m_pos; m_pos = p; Invalidate(); return o; }
	void ShimPaint(CDC &dc) override;
};
class CListCtrl : public CWnd {
public:
	struct Col { CString text; int cx; int fmt; };
	struct Row { std::vector<CString> text; int image; LPARAM data; UINT state; };
	std::vector<Col> m_cols;
	std::vector<Row> m_rows;
	CImageList *m_pSmall = nullptr;
	int m_top = 0, m_left = 0;
	BOOL m_dragThumb = FALSE;
	BOOL Create(DWORD style, const RECT &r, CWnd *parent, UINT id);
	int InsertColumn(int i, LPCTSTR text, int fmt = LVCFMT_LEFT, int width = -1, int sub = -1);
	BOOL DeleteColumn(int i) { if (i < (int)m_cols.size()) m_cols.erase(m_cols.begin() + i); Invalidate(); return TRUE; }
	int GetColumnWidth(int i) const { return i < (int)m_cols.size() ? m_cols[i].cx : 0; }
	BOOL SetColumnWidth(int i, int cx) { if (i < (int)m_cols.size()) m_cols[i].cx = cx; Invalidate(); return TRUE; }
	CImageList *SetImageList(CImageList *il, int which) { CImageList *o = m_pSmall; if (which == LVSIL_SMALL) m_pSmall = il; return o; }
	int InsertItem(int i, LPCTSTR text) { return InsertItem(i, text, -1); }
	int InsertItem(int i, LPCTSTR text, int image);
	int InsertItem(const LVITEM *it);
	int InsertItem(UINT mask, int i, LPCTSTR text, UINT state, UINT stateMask, int image, LPARAM lp);
	BOOL SetItemText(int i, int sub, LPCTSTR text);
	CString GetItemText(int i, int sub) const;
	int GetItemText(int i, int sub, LPTSTR buf, int n) const;
	BOOL SetItem(const LVITEM *it);
	BOOL SetItem(int i, int sub, UINT mask, LPCTSTR text, int image, UINT state, UINT stateMask, LPARAM lp);
	BOOL GetItem(LVITEM *it) const;
	BOOL SetItemData(int i, DWORD_PTR d) { if (i < 0 || i >= GetItemCount()) return FALSE; m_rows[i].data = d; return TRUE; }
	DWORD_PTR GetItemData(int i) const { return (i < 0 || i >= GetItemCount()) ? 0 : m_rows[i].data; }
	BOOL SetItemState(int i, UINT state, UINT mask);
	UINT GetItemState(int i, UINT mask) const { return (i < 0 || i >= GetItemCount()) ? 0 : (m_rows[i].state & mask); }
	int GetNextItem(int start, int flags) const;
	int GetItemCount() const { return (int)m_rows.size(); }
	int GetSelectedCount() const;
	POSITION GetFirstSelectedItemPosition() const { int i = GetNextItem(-1, LVNI_SELECTED); return (POSITION)(uintptr_t)(i + 1); }
	int GetNextSelectedItem(POSITION &p) const { int i = (int)(uintptr_t)p - 1; int n = GetNextItem(i, LVNI_SELECTED); p = (POSITION)(uintptr_t)(n + 1); return i; }
	BOOL DeleteItem(int i);
	BOOL DeleteAllItems();
	BOOL EnsureVisible(int i, BOOL partial);
	int FindItem(LVFINDINFO *fi, int start = -1) const;
	BOOL SortItems(PFNLVCOMPARE fn, DWORD_PTR data);
	BOOL Update(int) { Invalidate(); return TRUE; }
	int GetTopIndex() const { return m_top; }
	int GetCountPerPage() const;
	DWORD SetExtendedStyle(DWORD) { return 0; }
	BOOL SetBkColor(COLORREF) { return TRUE; }
	void ShimPaint(CDC &dc) override;
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimKey(UINT vk, BOOL down) override;
	BOOL ShimWheel(int dy) override;
	BOOL ShimWantsFocus() const override { return IsWindowEnabled(); }
	void Select(int i, BOOL notify = TRUE);
	void Notify(UINT code, int item, UINT newState = 0, UINT oldState = 0, int sub = 0);
	int HeaderH() const;
	int RowH() const;
	BOOL NeedVScroll() const;
};
class CToolTipCtrl : public CWnd {};

// scrolling window used by the map and message views
class CScrollView : public CWnd {
public:
	CSize m_totalLog;
	CPoint m_scroll;
	int m_drag = 0;          // 1 vertical thumb, 2 horizontal thumb
	int m_dragOff = 0;
	void SetScrollSizes(int mode, SIZE total, const SIZE & = CSize(0, 0), const SIZE & = CSize(0, 0));
	void ScrollToPosition(POINT pt);
	CPoint GetScrollPosition() const { return m_scroll; }
	CPoint GetDeviceScrollPosition() const { return m_scroll; }
	void GetScrollBarSizes(CSize &s) { s.cx = GetSystemMetrics_(SM_CXVSCROLL); s.cy = GetSystemMetrics_(SM_CYHSCROLL); }
	CSize GetTotalSize() const { return m_totalLog; }
	virtual void OnDraw(CDC *pDC) {}
	void OnPrepareDC(CDC *pDC);
	afx_msg void OnPaint();
	BOOL ShimMouse(UINT msg, CPoint pt) override;
	BOOL ShimWheel(int dy) override;
	void ShimPaint(CDC &dc) override;
	BOOL HasV() const;
	BOOL HasH() const;
	int ViewW() const;
	int ViewH() const;
	static int GetSystemMetrics_(int i);
	void Clamp();
	DECLARE_MESSAGE_MAP()
};
typedef CScrollView CView;

class CMenu : public CObject {
public:
	struct Item { CString text; UINT id; };
	std::vector<Item> m_items;
	CMenu *m_sub = nullptr;
	~CMenu() { delete m_sub; }
	BOOL LoadMenu(UINT id);
	CMenu *GetSubMenu(int) { return m_sub; }
	BOOL CreatePopupMenu() { return TRUE; }
	BOOL AppendMenu(UINT, UINT_PTR id, LPCTSTR text) { m_items.push_back({text, (UINT)id}); return TRUE; }
	BOOL TrackPopupMenu(UINT flags, int x, int y, CWnd *owner, LPCRECT = NULL);
	BOOL DestroyMenu() { return TRUE; }
};

class CFileDialog : public CDialog {
public:
	BOOL m_bOpen;
	CString m_ext, m_path;
	CFileDialog(BOOL open, LPCTSTR ext = NULL, LPCTSTR name = NULL, DWORD flags = 0, LPCTSTR filter = NULL, CWnd *parent = NULL);
	INT_PTR DoModal() override;
	CString GetPathName() const { return m_path; }
	CString GetFileName() const;
};

class CWinApp : public CCmdTarget {
	DECLARE_MESSAGE_MAP()
public:
	CWnd *m_pMainWnd = nullptr;
	const char *m_pszAppName = "Decker";
	CWinApp();
	virtual BOOL InitInstance() { return TRUE; }
	virtual int ExitInstance() { return 0; }
	virtual int Run();
	HICON LoadIcon(UINT) const { return nullptr; }
	HICON LoadIcon(LPCTSTR) const { return nullptr; }
	HICON LoadStandardIcon(LPCTSTR) const { return nullptr; }
	HCURSOR LoadStandardCursor(LPCTSTR) const { return nullptr; }
	void Enable3dControls() {}
	void Enable3dControlsStatic() {}
	void OnHelp();
	void WinHelp(DWORD data, UINT cmd = HELP_CONTEXT);
	CString GetProfileString(LPCTSTR, LPCTSTR, LPCTSTR def = NULL) { return def; }
	int GetProfileInt(LPCTSTR, LPCTSTR, int def) { return def; }
	BOOL WriteProfileString(LPCTSTR, LPCTSTR, LPCTSTR) { return TRUE; }
	BOOL WriteProfileInt(LPCTSTR, LPCTSTR, int) { return TRUE; }
};

// ---------------------------------------------------------------- DDX
void DDX_Control(CDataExchange *pDX, int id, CWnd &w);
void DDX_Text(CDataExchange *pDX, int id, CString &v);
void DDX_Text(CDataExchange *pDX, int id, int &v);
void DDX_Text(CDataExchange *pDX, int id, UINT &v);
void DDX_Text(CDataExchange *pDX, int id, long &v);
void DDX_Text(CDataExchange *pDX, int id, short &v);
void DDX_Text(CDataExchange *pDX, int id, BYTE &v);
void DDX_Text(CDataExchange *pDX, int id, double &v);
void DDX_Check(CDataExchange *pDX, int id, int &v);
void DDX_Radio(CDataExchange *pDX, int id, int &v);
void DDX_CBIndex(CDataExchange *pDX, int id, int &v);
void DDX_LBIndex(CDataExchange *pDX, int id, int &v);
void DDX_CBString(CDataExchange *pDX, int id, CString &v);
void DDX_LBString(CDataExchange *pDX, int id, CString &v);
void DDV_MaxChars(CDataExchange *pDX, CString const &v, int n);
void DDV_MinMaxInt(CDataExchange *pDX, int v, int lo, int hi);
void DDV_MinMaxUInt(CDataExchange *pDX, UINT v, UINT lo, UINT hi);

// ---------------------------------------------------------------- globals
CWinApp *AfxGetApp();
CWnd *AfxGetMainWnd();
HINSTANCE AfxGetInstanceHandle();
HINSTANCE AfxGetResourceHandle();
LPCTSTR AfxRegisterWndClass(UINT, HCURSOR = 0, HBRUSH = 0, HICON = 0);
int AfxMessageBox(LPCTSTR text, UINT type = MB_OK, UINT help = 0);
int AfxMessageBox(UINT id, UINT type = MB_OK, UINT help = 0);
int MessageBox(HWND, LPCTSTR text, LPCTSTR caption, UINT type);
int GetSystemMetrics(int i);
DWORD GetTickCount();
void Sleep(DWORD ms);
BOOL PlaySound(LPCSTR file, HINSTANCE, DWORD flags);
DWORD GetPrivateProfileString(LPCTSTR sec, LPCTSTR key, LPCTSTR def, LPTSTR out, DWORD n, LPCTSTR file);
UINT GetPrivateProfileInt(LPCTSTR sec, LPCTSTR key, int def, LPCTSTR file);
BOOL WritePrivateProfileString(LPCTSTR sec, LPCTSTR key, LPCTSTR val, LPCTSTR file);
LONG RegOpenKeyEx(HKEY, LPCTSTR, DWORD, DWORD, HKEY *);
LONG RegQueryValueEx(HKEY, LPCTSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
LONG RegCloseKey(HKEY);
HANDLE LoadImage(HINSTANCE, LPCTSTR name, UINT type, int cx, int cy, UINT flags);
HBITMAP LoadBitmap(HINSTANCE, LPCTSTR name);
HANDLE CopyImage(HANDLE h, UINT type, int cx, int cy, UINT flags);
HCURSOR LoadCursor(HINSTANCE, LPCTSTR);
HACCEL LoadAccelerators(HINSTANCE, LPCTSTR id);
int TranslateAccelerator(HWND, HACCEL, LPMSG);
BOOL DeleteObject(HGDIOBJ);
int GetObject(HGDIOBJ, int, LPVOID);
int GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, BITMAPINFO *, UINT);
BOOL GetBitmapDimensionEx(HBITMAP, LPSIZE);
HPALETTE CreatePalette(const LOGPALETTE *);
BOOL GetCursorPos(LPPOINT);
int GetDlgCtrlID(HWND h);
HWND GetActiveWindow();
HWND GetParent(HWND h);
BOOL IsWindowEnabled(HWND h);
BOOL EnableWindow(HWND h, BOOL e);
BOOL IsWindowVisible(HWND h);
void PostQuitMessage(int);
short GetKeyState(int vk);
short GetAsyncKeyState(int vk);
int LoadString(HINSTANCE, UINT id, LPTSTR buf, int n);
LRESULT SendMessage(HWND h, UINT msg, WPARAM wp, LPARAM lp);
BOOL PostMessage(HWND h, UINT msg, WPARAM wp, LPARAM lp);
HGDIOBJ SelectObject(HDC, HGDIOBJ);
#define ZeroMemory(p, n) memset((p), 0, (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#define lstrcpy strcpy
#define lstrlen strlen
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _snprintf snprintf
#define _itoa(v, b, r) (sprintf((b), "%d", (v)), (b))
#define itoa _itoa

// shim internals shared with the frontend
namespace shim {
extern std::vector<CWnd *> g_topLevel;   // z-order, back to front
extern CWnd *g_focus;
extern CWnd *g_capture;
extern BOOL g_quit;
std::string ResolvePath(const char *path);  // backslashes, case-insensitive lookup
Bitmap *LoadBmpFile(const char *path);
Bitmap *LoadBmpRes(UINT id);
void PaintDirty();
void Present();
void PumpEvents(BOOL wait);
void Idle(int ms);
CWnd *ModalTop();
void HandleKey(UINT vk, BOOL down, UINT mods);
void HandleChar(UINT ch);
void HandleMouse(UINT msg, int x, int y);
void HandleWheel(int x, int y, int dy);
extern int g_mouseX, g_mouseY;
int TextWidth(int font, const char *s, int n);
int FontHeight(int font);
void DrawText(CDC &dc, int font, int x, int y, const char *s, int n, COLORREF c);
void DrawFrameCtl(CDC &dc, CRect r, BOOL sunken, BOOL thick = TRUE);
void DrawArrow(CDC &dc, CRect r, int dir, COLORREF c);
void AddTopLevel(CWnd *w);
void RemoveTopLevel(CWnd *w);
extern std::string g_saveDir;
void SyncFS();
}

#define COLOR_FACE RGB(192, 192, 192)
#define COLOR_SHADOW RGB(128, 128, 128)
#define COLOR_HILITE RGB(255, 255, 255)
#define COLOR_DKSHADOW RGB(0, 0, 0)
#define COLOR_LIGHT RGB(223, 223, 223)
#define COLOR_SELBG RGB(0, 0, 128)

#endif
