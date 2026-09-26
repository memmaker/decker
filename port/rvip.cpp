// RVIP additions: Enter command menu (home and Matrix), auto-explore (X) in
// the Matrix, numeric keypad movement. Hooked from the frontend's key handler.
#include "stdafx.h"
#include "Decker.h"
#include "HomeView.h"
#include "MatrixView.h"
#include "Character.h"
#include "System.h"
#include "Area.h"
#include "Node.h"
#include "Ice.h"
#include "Global.h"
#include <set>
#include <map>
#include <deque>

extern CCharacter *g_pChar;

#define ID_RVIP_EXPLORE 0xE001
#define ID_RVIP_HELP 0xE002

struct Cmd { UINT id; const char *text; };
static const Cmd g_matrixMenu[] = {
	{0, "Movement"},
	{IDC_MATRIX_N, "Move north\tUp"}, {IDC_MATRIX_E, "Move east\tRight"}, {IDC_MATRIX_S, "Move south\tDown"}, {IDC_MATRIX_W, "Move west\tLeft"},
	{IDC_WAIT1, "Wait one turn\tW"}, {ID_RVIP_EXPLORE, "Explore this area\tX"},
	{0, "Combat"},
	{IDC_ATTACK, "Attack (default program)\tA"}, {IDC_DECEIVE, "Deceive\tD"}, {IDC_ANALYZE, "Analyze\tZ"},
	{IDC_NEXT_TARGET, "Next target\tTab"}, {IDC_VIEW_ICE, "View target ICE\tV"},
	{0, "Programs"},
	{IDC_SCAN, "Scan node\tS"}, {IDC_EVALUATE, "Evaluate files\tE"}, {IDC_DECRYPT, "Decrypt\tC"}, {IDC_RELOCATE, "Relocate trace\tR"},
	{IDC_SILENCE, "Silence\tL"}, {IDC_SMOKE, "Smoke\tK"}, {IDC_MEDIC, "Medic\tM"},
	{IDC_RUN_PROGRAM, "Run selected program"}, {IDC_SET_DEF_PROGRAM, "Set default attack program"},
	{IDC_LOAD_PROGRAM, "Load a program"}, {IDC_UNLOAD_PROGRAM, "Unload selected program"},
	{0, "Node"},
	{IDC_GET_FILE, "Get a file"}, {IDC_EDIT_FILE, "Edit a file"}, {IDC_ERASE_FILE, "Erase a file"}, {IDC_CRASH, "Crash the system"},
	{IDC_USE_IO, "Activate I/O"}, {IDC_GET_MAP, "Get area map"}, {IDC_BACKDOOR, "Create a backdoor"}, {IDC_KILL_ALARM, "Cancel alert"},
	{IDC_ENTER_PORTAL, "Enter portal"}, {IDC_KILL_TRACE, "Cancel trace"}, {IDC_KILL_SHUTDOWN, "Cancel shutdown"},
	{0, "Information"},
	{IDC_MATRIX_CHAR, "Character"}, {IDC_MATRIX_DECK, "Cyberdeck"}, {IDC_MATRIX_CONTRACT, "Contract"}, {IDC_MAP_ZOOM, "Zoom the map"},
	{0, "Game"},
	{IDC_OPTIONS, "Options (save, load, quit)"}, {IDC_QUICK_SAVE_KEY, "Quick save\tCtrl+S"}, {IDC_QUICK_LOAD_KEY, "Quick load\tCtrl+L"},
	{IDC_MATRIX_DISCONNECT, "Disconnect"}, {ID_RVIP_HELP, "Help\tF1"},
};
static const Cmd g_homeMenu[] = {
	{0, "At home"},
	{IDC_HOME_CHAR, "View character\tC"}, {IDC_HOME_DECK, "View cyberdeck\tD"}, {IDC_HOME_CONTRACT, "Contracts\tK"},
	{IDC_HOME_BUY, "Buy hardware/software\tB"}, {IDC_HOME_PROGRAM, "Projects\tP"}, {IDC_HOME_REST, "Rest and recuperate\tR"},
	{IDC_HOME_MATRIX, "Enter the Matrix\tM"}, {IDC_OPTIONS, "Game options\tO"}, {ID_RVIP_HELP, "Help\tF1"},
};

static UINT CommandMenu(CWnd *view, const Cmd *cmds, int n)
{
	CMenu m;
	for (int i = 0; i < n; i++) {
		if (cmds[i].id && cmds[i].id < 0xE000) {
			// buttons that are hidden or greyed out right now are left out
			CWnd *b = view->GetDlgItem(cmds[i].id);
			if (b && (!b->IsWindowVisible() || !b->IsWindowEnabled())) continue;
		}
		if (!cmds[i].id && !m.m_items.empty() && !m.m_items.back().id) m.m_items.pop_back(); // empty group
		m.AppendMenu(MF_STRING, cmds[i].id, cmds[i].text);
	}
	if (!m.m_items.empty() && !m.m_items.back().id) m.m_items.pop_back();
	int h = 0;
	for (auto &it : m.m_items) h += shim::FontHeight(0) + 5;
	return m.TrackPopupMenu(TPM_RETURNCMD, 220, (480 - h) / 2 - 3, view);
}

// ---------------------------------------------------------------- auto-explore
static std::set<CNode *> g_visited;
static CSystem *g_visitedSys;

static void MarkVisited()
{
	if (g_visitedSys != g_pChar->m_pSystem) { g_visited.clear(); g_visitedSys = g_pChar->m_pSystem; }
	g_visited.insert(g_pChar->m_pCurrentNode);
}
// first step towards the nearest node of this area not stood on yet, through known nodes
static int ExploreDir()
{
	CNode *start = g_pChar->m_pCurrentNode;
	std::map<CNode *, int> firstDir;
	std::deque<CNode *> q;
	firstDir[start] = -1;
	q.push_back(start);
	while (!q.empty()) {
		CNode *n = q.front();
		q.pop_front();
		if (n != start && !g_visited.count(n)) return firstDir[n];
		if (n != start && !n->m_bMapped) continue; // unknown nodes are targets, not paths
		for (int d = 0; d < 4; d++) {
			CNode *a = n->m_pAdjNode[d];
			if (!a || a->m_pParentArea != start->m_pParentArea || firstDir.count(a)) continue;
			firstDir[a] = n == start ? d : firstDir[n];
			q.push_back(a);
		}
	}
	return -1;
}
static BOOL MatrixAlive(CMatrixView *v)
{
	for (CWnd *t : shim::g_topLevel) if (t == v) return g_pChar && g_pChar->m_bOnRun;
	return FALSE;
}
static void Explore(CMatrixView *v)
{
	static const UINT moveId[4] = {IDC_MATRIX_N, IDC_MATRIX_E, IDC_MATRIX_S, IDC_MATRIX_W};
	int input = shim::g_inputCount;
	shim::g_swallowInput = TRUE;
	for (int steps = 0; steps < 200; steps++) {
		MarkVisited();
		if (steps > 0 && !g_pChar->m_olCurrentIceList.IsEmpty()) break; // ICE here
		int d = ExploreDir();
		if (d < 0) {
			v->m_MessageView.AddMessage("Nothing left to explore in this area.", BLACK);
			break;
		}
		CNode *from = g_pChar->m_pCurrentNode;
		int msgs = v->m_MessageView.m_nAdded;
		v->SendMessage(WM_COMMAND, moveId[d], 0);
		if (!MatrixAlive(v)) break;
		MarkVisited();
		shim::PaintDirty();
		shim::Present();
		shim::PumpEvents(FALSE);
		shim::Idle(120);
		shim::PumpEvents(FALSE);
		if (g_pChar->m_pCurrentNode == from) break; // blocked (gateway, ...)
		// the move itself says "Entering node ..."; anything else stops
		int added = v->m_MessageView.m_nAdded - msgs;
		if (added > 1 || (added == 1 && v->m_MessageView.m_szLast.Find("Entering node") != 0)) break;
		if (shim::g_inputCount != input) break;
		if (shim::g_quit) break;
	}
	shim::g_swallowInput = FALSE;
}

// ---------------------------------------------------------------- key hook
BOOL port_key(CWnd *top, UINT vk, UINT mods)
{
	if (!top || !g_pChar) return FALSE;
	if (CMatrixView *v = dynamic_cast<CMatrixView *>(top)) {
		CWnd *f = CWnd::GetFocus();
		if (f && dynamic_cast<CEdit *>(f)) return FALSE;
		UINT id = 0;
		switch (vk) {
		case VK_NUMPAD0 + 8: id = IDC_MATRIX_N; break;
		case VK_NUMPAD0 + 6: id = IDC_MATRIX_E; break;
		case VK_NUMPAD0 + 2: id = IDC_MATRIX_S; break;
		case VK_NUMPAD0 + 4: id = IDC_MATRIX_W; break;
		case VK_NUMPAD0 + 5: id = IDC_WAIT1; break;
		case 'X': if (mods) return FALSE; id = ID_RVIP_EXPLORE; break;
		case VK_RETURN: id = CommandMenu(v, g_matrixMenu, sizeof g_matrixMenu / sizeof *g_matrixMenu); if (!id) return TRUE; break;
		default: return FALSE;
		}
		if (id == ID_RVIP_EXPLORE) Explore(v);
		else if (id == ID_RVIP_HELP) { HELPINFO hi = {sizeof hi}; v->SendMessage(WM_HELP, 0, (LPARAM)&hi); }
		else if (CWnd *b = v->GetDlgItem(id); b && !b->IsWindowEnabled()) {}
		else v->SendMessage(WM_COMMAND, id, 0);
		return TRUE;
	}
	if (CHomeView *h = dynamic_cast<CHomeView *>(top)) {
		if (vk != VK_RETURN) return FALSE;
		UINT id = CommandMenu(h, g_homeMenu, sizeof g_homeMenu / sizeof *g_homeMenu);
		if (id == ID_RVIP_HELP) { HELPINFO hi = {sizeof hi}; h->SendMessage(WM_HELP, 0, (LPARAM)&hi); }
		else if (id) h->SendMessage(WM_COMMAND, id, 0);
		return TRUE;
	}
	return FALSE;
}

// Run report (roguelikes-index/server/CONTRACT.md): fire-and-forget GET,
// never throws, offline just fails silently. Negative ints are omitted.
// Killer = the ICE type name of the last ICE that did damage ("Name 1A2F"
// instance suffix dropped). No score, turns or depth: the game keeps none.
static char g_szRvipKiller[64];
void rvipLastHit(const char *szIceName)
{
	snprintf(g_szRvipKiller, sizeof g_szRvipKiller, "%s", szIceName);
	char *sp = strrchr(g_szRvipKiller, ' ');
	if (sp && strlen(sp) == 5) *sp = 0;
}
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, js_beacon, (const char *g, const char *ev, const char *name, const char *killer, int depth, int score, int turns, int lvl), {
    try {
        var p = [['g', UTF8ToString(g)], ['ev', UTF8ToString(ev)], ['name', name ? UTF8ToString(name) : ''],
                 ['killer', killer ? UTF8ToString(killer) : ''], ['depth', depth], ['score', score], ['turns', turns], ['lvl', lvl]];
        var q = p.filter(function (a) { return a[1] !== '' && !(a[1] < 0); })
                 .map(function (a) { return a[0] + '=' + encodeURIComponent(a[1]); }).join('&');
        fetch('/roguelikes/beacon?' + q, { keepalive: true, mode: 'no-cors' }).catch(function () {});
    } catch (e) {}
});
#endif
// ev: "death" (killer NULL = last ICE hit), "quit"; szKiller overrides.
void rvipRunEnd(const char *ev, const char *szKiller)
{
#ifdef __EMSCRIPTEN__
	if (!g_pChar) return;
	if (!szKiller && !strcmp(ev, "death") && g_szRvipKiller[0]) szKiller = g_szRvipKiller;
	js_beacon("decker", ev, (LPCTSTR)g_pChar->m_szName, szKiller, -1, -1, -1, g_pChar->m_nRepLevel);
#endif
}
