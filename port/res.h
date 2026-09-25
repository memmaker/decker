// resource tables generated from Decker.rc by port/rc2res.py
#ifndef PORT_RES_H
#define PORT_RES_H
struct DlgItem { const char *cls; const char *text; int id; int x, y, cx, cy; DWORD style, exStyle; };
struct DlgTemplate { int id; int cx, cy; const char *caption; DWORD style; const DlgItem *items; int count; };
struct BitmapRes { int id; const char *file; };
struct AccelRes { int table; int key; int cmd; int flags; };
struct StringRes { int id; const char *text; };
struct MenuRes { int menu; const char *text; int id; };
struct DlgInitRes { int dlg; int ctl; const char *text; };
extern const DlgTemplate g_dlgTemplates[];
extern const BitmapRes g_bitmapRes[];
extern const AccelRes g_accelRes[];
extern const StringRes g_stringRes[];
extern const MenuRes g_menuRes[];
extern const DlgInitRes g_dlgInitRes[];
#endif
