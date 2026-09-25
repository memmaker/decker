#!/usr/bin/env python3
"""Help/Decker.rtf (WinHelp source) -> doc/index.html.
Topics start at a '#' footnote (context id = anchor); '$' is the title, 'K'/'A'
footnotes are dropped. Double-underlined text followed by hidden text is a jump
(the hidden text is the target id). {bmc x.bmp} becomes an image (converted to PNG).
Usage: rtf2html.py Help/Decker.rtf doc"""
import re, sys, os, html, subprocess

src, out = sys.argv[1], sys.argv[2]
s = open(src, 'rb').read().decode('cp1252')
os.makedirs(out, exist_ok=True)

tok = re.compile(r"\\([a-z]+)(-?\d+)? ?|\\'([0-9a-f]{2})|\\([^a-z])|([{}])|([^\\{}\r\n]+)|[\r\n]+")
SKIP = {'fonttbl', 'colortbl', 'stylesheet', 'info', 'listtable', 'listoverridetable',
		'rsidtbl', 'generator', 'pict', 'object', 'latentstyles', 'xmlnstbl', 'datastore', 'themedata'}
state = dict(ul=False, v=False, skip=False, fn=None, b=False)
stack = []
topics = []       # [id, title, html]
cur = None
fn_text = ''
link_text = ''
hid = ''
para = ''

def emit(t):
	global para, link_text, fn_text, hid
	if state['skip']: return
	if state['fn'] is not None: fn_text += t; return
	if state['v']:
		if link_text: hid += t    # jump target, may come in several groups
		return
	if state['ul']:
		if hid: flush_link()
		link_text += t; return
	flush_link()
	para += html.escape(t)

def flush_link():
	global link_text, para, hid
	if link_text and hid:
		para += '\0%s\1%s\2' % (hid.strip(), html.escape(link_text))
	elif link_text:
		para += html.escape(link_text)
	link_text = hid = ''

def end_para():
	global para
	flush_link()
	p = para.strip()
	para = ''
	if cur is None or not p or p == html.escape(cur[1]): return
	p = re.sub(r'\{bm[clr] ([\w]+)\.bmp\}', r'<img src="\1.png" alt="">', p)
	if p.startswith('- '):
		cur[2].append('<li>%s</li>' % p[2:])
	else:
		cur[2].append('<p>%s</p>' % p)

for m in tok.finditer(s):
	word, arg, hexc, sym, brace, text = m.groups()
	if brace == '{':
		stack.append(dict(state)); state['_new'] = True; continue
	if brace == '}':
		if state['fn'] is not None and (not stack or stack[-1]['fn'] is None):
			kind, t = state['fn'], fn_text.strip()[1:].strip()
			if kind == '#': end_para(); cur = [t, '', []]; topics.append(cur)
			elif kind == '$' and cur: cur[1] = t
			fn_text = ''
		new = stack.pop() if stack else state
		if state['ul'] and not new['ul'] and not new['v']: pass
		state.clear(); state.update(new); continue
	if word:
		if state.get('_new') and word in SKIP: state['skip'] = True
		state['_new'] = False
		if word == 'footnote': state['fn'] = para[-1:] or '?'; para = para[:-1]; fn_text = ''
		elif word in ('par', 'line'): end_para() if state['fn'] is None else None
		elif word == 'page': end_para()
		elif word == 'uldb' or word == 'ul': state['ul'] = arg != '0'
		elif word == 'ulnone': state['ul'] = False
		elif word == 'v': state['v'] = arg != '0'
		elif word == 'plain': state['ul'] = state['v'] = False
		elif word == 'tab': emit(' ')
		elif word in ('rquote', 'lquote'): emit("'")
		elif word in ('rdblquote', 'ldblquote'): emit('"')
		elif word in ('endash', 'emdash'): emit('-')
		elif word == 'bullet': emit('- ')
		continue
	state['_new'] = False
	if hexc: emit(bytes([int(hexc, 16)]).decode('cp1252'))
	elif sym:
		if sym == '*': state['skip'] = True
		elif sym in '\\{}': emit(sym)
		elif sym == '~': emit(' ')
	elif text:
		# the footnote marker characters (#, $, K, A) sit in their own group before \footnote
		emit(text)
end_para()

topics.append(['rvip', 'Added in this version (keys)', [
	'<p>This is a port of Decker 1.12 to macOS and the web. It adds:</p><ul>'
	'<li><b>Enter</b>: a command menu with every button of the current screen (at home and in the Matrix), grouped; '
	'arrow keys or mouse to pick, the key shown next to an entry also works, Escape closes.</li>'
	'<li><b>X</b> (in the Matrix): explore. Walks node by node to the nearest node of this area you have not '
	'stood on yet; stops when ICE is present, on any message or any key.</li>'
	'<li>Numeric keypad 8/6/2/4 moves, 5 waits.</li>'
	'<li>There are no stairs and no item inventory in Decker, so the usual stair-walking and item menus do not apply.</li></ul>']])
ids = {t[0].lower(): t[0] for t in topics}
def link(m):
	target, text = m.group(1), m.group(2)
	if 'EF(' in target:   # WinHelp ExecFile macro: a web address or mail link
		plain = html.unescape(text)
		href = 'mailto:' + plain if '@' in plain else plain if '://' in plain else 'https://' + plain
		return '<a href="%s">%s</a>' % (html.escape(href), text)
	t = ids.get(target.lower())
	if not t: print('unresolved jump:', target, file=sys.stderr)
	return '<a href="#%s">%s</a>' % (html.escape(t), text) if t else text
for t in topics:
	t[2] = [re.sub('\0(.*?)\1(.*?)\2', link, x) for x in t[2]]
toc = ''.join('<li><a href="#%s">%s</a></li>' % (html.escape(t[0]), html.escape(t[1] or t[0])) for t in topics)
# numeric anchors for the WinHelp(HID_*) context ids the game passes to port_help()
hm = dict(re.findall(r'#define\s+(\w+)\s+(\d+)', open(os.path.join(os.path.dirname(src), '..', 'Decker.hm')).read()))
body = ''.join('<section id="%s"><h2 id="h%s">%s</h2>%s</section>' % (html.escape(t[0]), hm.get(t[0], ''), html.escape(t[1] or t[0]),
	re.sub(r'((?:<li>.*?</li>)+)', r'<ul>\1</ul>', ''.join(t[2]))) for t in topics)
open(os.path.join(out, 'index.html'), 'w').write('''<!doctype html><meta charset="utf-8"><title>Decker Help</title>
<style>body{font:15px/1.5 sans-serif;max-width:52em;margin:1em auto;padding:0 16px;background:#fff;color:#111}
img{max-width:100%%;image-rendering:pixelated}section{border-top:1px solid #ccc;margin-top:1.5em}</style>
<h1>Decker 1.12 Help</h1><ul>%s</ul>%s''' % (toc, body))
for b in set(re.findall(r'bmc ([\w.]+)', s)):
	p = os.path.join(os.path.dirname(src), b)
	if os.path.exists(p):
		subprocess.run([sys.executable, '-c', 'import sys;from PIL import Image;Image.open(sys.argv[1]).save(sys.argv[2])',
			p, os.path.join(out, b.rsplit('.', 1)[0] + '.png')], check=True)
print(len(topics), 'topics')
