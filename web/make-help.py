#!/usr/bin/env python3
"""Writes the in-page game guide (dist/help.html) for the web build.

The game content comes from the desktop key guides in
~/Desktop/Games/Roguelikes/Docs (build-docs.py + guides.py), so both guides
stay in sync; only the saving and "playing in the browser" parts are
written here, because they differ on the web."""
import html, importlib.util, os, sys

DOCS = os.path.expanduser('~/Desktop/Games/Roguelikes/Docs')
PAGE = 'decker.html'

sys.path.insert(0, DOCS)
spec = importlib.util.spec_from_file_location('build_docs', os.path.join(DOCS, 'build-docs.py'))
docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)
from guides import GUIDES   # noqa: E402

game = next(g for g in docs.GAMES if g['file'] == PAGE)
guide = dict(GUIDES.get(PAGE, {}))
info = dict(game['info'])
kbd = docs.kbd
esc = html.escape

SAVING = '''<ul>
<li>Save with <strong>Options → Save As…</strong> (the Options button, or <kbd>Enter</kbd> → Options). In the Matrix, <kbd>Ctrl</kbd>+<kbd>S</kbd> quick-saves to the last file and <kbd>Ctrl</kbd>+<kbd>L</kbd> quick-loads it. Load from the title screen (<em>Load</em>) or Options → <em>Load Game</em>.</li>
<li>Save files (<code>.DSG</code>) are kept in this browser (IndexedDB) and survive a reload. <strong>There is no autosave</strong>: save before you close the tab (the browser warns you).</li>
<li><em>Export save</em> downloads your newest save; <em>Import save</em> adds a <code>.DSG</code> file (also one from the Windows or Mac version), which you then load in the game.</li>
<li>In Ironman mode the game does not let you save in the Matrix.</li>
<li>Private/incognito windows and "clear site data" delete the stored saves. Export first if they matter.</li>
</ul>'''

WEB = '''<ul>
<li>The original 640×480 Windows screen, scaled to the window with sharp pixels. Resize the browser window to change the size.</li>
<li>Everything works with the mouse, as in the original. <kbd>Enter</kbd> opens a menu of every command on the current screen.</li>
<li><em>Sound</em> in the top bar turns the game's sound effects on (off by default). The game's own Options → Sound Effects setting also applies.</li>
<li><kbd>F1</kbd> opens the original Decker help file at the topic of the current screen; in Help, <em>Manual</em> opens it at its contents and <em>Guide</em> comes back here. <kbd>Esc</kbd> or <em>Close</em> returns to the game.</li>
<li>Browsers keep a few shortcuts for themselves (<kbd>Ctrl+W</kbd>, <kbd>Ctrl+T</kbd>, <kbd>Cmd</kbd> shortcuts on a Mac).</li>
<li>If the game ever crashes, a message appears at the top; reload the page and load your last save.</li>
</ul>'''

KEY_HINTS = [
    ('Enter', 'Menu of all commands on this screen'),
    ('X', 'Explore (in the Matrix): walk to the nearest node of this area not visited yet'),
    ('F1', 'Help for the current screen'),
    ('Arrow keys', 'Move in the Matrix (keypad 8 6 2 4 too; W or 5 waits)'),
    ('^S', 'Quick save (in the Matrix)'),
]


def dl(items):
    return '<dl>' + ''.join(f'<dt>{kbd(k)}</dt><dd>{esc(d)}</dd>' for k, d in items) + '</dl>'


def section(anchor, title, body):
    return f'<h2 id="h-{anchor}">{esc(title)}</h2>{body}'


parts = []
toc = [('about', 'About the game'), ('keys', 'Keyboard controls'), ('saving', 'Saving your game'),
       ('tips', 'Tips'), ('guide', "New player's guide"), ('web', 'Playing in the browser'), ('manual', 'Manual')]
parts.append('<p>' + esc(game['tagline']) + '</p>' + info['About the game'] + '<ul class="toc">' +
             ''.join(f'<li><a href="#h-{a}">{esc(t)}</a></li>' for a, t in toc) + '</ul>')


parts.append(section('about', 'About the game',
                     guide.pop('How Decker differs from other roguelikes')))

ess = ''.join(f'<div class="box"><h3>{esc(cat)}</h3>{dl(items)}</div>' for cat, items in game['essentials'])
parts.append(section('keys', 'Keyboard controls',
                     '<div class="box key"><h3>The keys to remember</h3>' + dl(KEY_HINTS) + '</div>'
                     '<h3>All keys</h3><div class="grid">' + ess + '</div>'
                     '<p>These are all of Decker\'s keys; everything else is a button. There are no stairs and no item inventory, '
                     'so the stair-walking and item menus of the other games here don\'t apply.</p>'))

parts.append(section('saving', 'Saving your game', SAVING))
parts.append(section('tips', 'Tips', info['Tips']))
parts.append(section('guide', "New player's guide",
                     ''.join(f'<h3>{esc(t)}</h3>{b}' for t, b in guide.items())))
parts.append(section('web', 'Playing in the browser', WEB))
parts.append(section('manual', 'Manual',
                     '<p>The original Decker help file by Shawn Overcash (<code>Help/Decker.rtf</code>, converted to HTML): every screen, '
                     'program and rule in detail. <a href="doc/index.html" data-manual>Open the Decker manual</a> '
                     '(<kbd>F1</kbd> in the game opens it at the current screen).</p>'))

# RVIP: About this version
parts.append('<h2 id="h-version">About this version</h2><ul>'
             '<li>Based on <strong>Decker 1.12</strong> by Shawn Overcash (GPL; source archive <code>DeckerSource_1_12.zip</code>, '
             'sha256 <code>45588c17…1f8555</code>, from <a href="https://sourceforge.net/projects/decker/">sourceforge.net/projects/decker</a>): '
             '<a href="https://github.com/memmaker/decker/tree/c61cf6e">untouched source</a>.</li>'
             '<li>Our changes: an MFC/Win32 shim and SDL2 frontend (so the Windows game runs on the Mac and in the browser), '
             'the Enter command menu, auto-explore, keypad movement, the help file converted to HTML, and this web page. '
             '<a href="https://github.com/memmaker/decker">github.com/memmaker/decker</a> '
             '(<a href="https://github.com/memmaker/decker/compare/c61cf6e...main">all changes</a>).</li></ul>')
print('\n'.join(parts))
