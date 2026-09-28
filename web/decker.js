/*
 * Decker in the browser: the SDL2 build (port/fe_sdl.cpp) draws the 640x480
 * screen into #canvas; this file scales it (nearest-neighbour), keeps the
 * save folder in IndexedDB (IDBFS, /decker/save), and runs the top bar:
 * help, File ▾ (export/import/new game), Audio ▾ (sound, off by default). Loaded before decker-core.js.
 * Page layout copied from ~/Games/omega/web.
 */
(function () {
	'use strict';

	var DIR = '/decker/save', SETTINGS = DIR + '/web-settings.json';
	var app, sound = false;

	function $(id) { return document.getElementById(id); }

	/* ---------- scaling: largest size that fits, whole pixels when it can ---------- */
	function fit() {
		var g = $('game'), cv = $('canvas');
		var s = Math.min(g.clientWidth / 640, g.clientHeight / 480);
		if (s >= 1) s = Math.floor(s * 4) / 4;   /* quarter steps, like the Mac window */
		cv.style.width = Math.floor(640 * s) + 'px';
		cv.style.height = Math.floor(480 * s) + 'px';
	}

	/* ---------- saves: IndexedDB (IDBFS) ---------- */
	function saves() {
		try {
			return Module.FS.readdir(DIR).filter(function (n) { return /\.dsg$/i.test(n); })
				.map(function (n) { return { name: n, t: Module.FS.stat(DIR + '/' + n).mtime.getTime() }; })
				.sort(function (a, b) { return b.t - a.t; });
		} catch (e) { return []; }
	}
	/* Export save: the newest .dsg; Import save adds the file (other saves stay) */
	function newestSave() { var s = saves()[0]; return s ? DIR + '/' + s.name : null; }
	function putSave(file, data) {
		var name = file.name.replace(/[^\w.-]/g, '_');
		if (!/\.dsg$/i.test(name)) name += '.DSG';
		Module.FS.writeFile(DIR + '/' + name, data);
	}

	/* ---------- sound (off by default, remembered in IndexedDB) ---------- */
	function setSound(on, store) {
		sound = on;
		$('chk-sound').checked = on;
		if (app.running) Module._web_set_sound(on ? 1 : 0);
		if (store) { try { Module.FS.writeFile(SETTINGS, JSON.stringify({ sound: on })); app.sync(); } catch (e) { } }
		/* browsers start audio only after a click */
		if (on && window.SDL2 && SDL2.audioContext && SDL2.audioContext.state === 'suspended') SDL2.audioContext.resume();
	}

	/* ---------- help: the guide (help.html) or the original manual (doc/) ---------- */
	var helpLoaded = false;
	function showHelp(manualCtx) {
		var h = $('help'), body = $('help-body'), fr = $('help-frame');
		h.hidden = false;
		if (manualCtx !== undefined) {
			$('help-title').textContent = 'Decker manual';
			$('help').firstElementChild.setAttribute('aria-label', 'Decker manual');
			$('help-switch').textContent = 'Guide'; $('help-switch').title = 'Back to the game guide';
			body.hidden = true; fr.hidden = false;
			fr.src = 'doc/index.html' + (manualCtx ? '#h' + manualCtx : '');
			fr.focus();
			return;
		}
		$('help-title').textContent = 'Decker guide';
		$('help').firstElementChild.setAttribute('aria-label', 'Decker guide');
		$('help-switch').textContent = 'Manual'; $('help-switch').title = 'The original Decker help file';
		body.hidden = false; fr.hidden = true;
		if (!helpLoaded) {
			helpLoaded = true;
			fetch('help.html').then(function (r) { if (!r.ok) throw new Error(r.status); return r.text(); })
				.then(function (t) { body.innerHTML = t; })
				.catch(function (err) { helpLoaded = false; body.textContent = 'Could not load the guide (' + err + ').'; });
		}
		body.focus();
	}
	function closeHelp() { $('help').hidden = true; $('canvas').focus(); }
	window.deckerHelp = function (ctx) { showHelp(ctx || 0); };
	window.deckerSync = function () { app.sync(); };
	window.deckerEnd = function () {
		app.running = false;
		app.sync();
		$('overlay').hidden = false;
	};
	/* while a panel is open the game gets no keys (SDL listens on window) */
	window.addEventListener('keydown', function (e) {
		if (!$('help').hidden) {
			if (e.key === 'Escape') closeHelp();
			e.stopImmediatePropagation();
		}
	}, true);
	window.addEventListener('keyup', function (e) { if (!$('help').hidden) e.stopImmediatePropagation(); }, true);

	/* ---------- startup ---------- */
	/* help stays here: it also shows the original manual (doc/) in an iframe; New game only restarts (saves are kept) */
	app = RvipApp({ name: 'decker', save: newestSave, clear: function () { }, put: putSave, newGame: function () { location.reload(); } });
	window.Module = {
		canvas: document.getElementById('canvas'),
		preRun: [function () {
			var FS = Module.FS;
			FS.mkdirTree(DIR);
			FS.mount(Module.IDBFS, {}, DIR);
			Module.addRunDependency('idbfs');
			FS.syncfs(true, function (err) {
				if (err) app.status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				try { sound = !!JSON.parse(FS.readFile(SETTINGS, { encoding: 'utf8' })).sound; } catch (e) { }
				Module.removeRunDependency('idbfs');
			});
		}],
		onRuntimeInitialized: function () {
			app.running = true; app.status('');
			$('game').hidden = false;
			setTimeout(function () { fit(); setSound(sound, false); $('canvas').focus(); }, 0);
		},
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !app.running) app.status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { app.crashed(what); }
	};
	document.addEventListener('visibilitychange', function () { if (document.hidden) app.sync(); });
	window.addEventListener('pagehide', function () { app.sync(); });
	window.addEventListener('beforeunload', function (e) { if (app.running) { e.preventDefault(); e.returnValue = ''; } });
	window.addEventListener('resize', fit);
	/* SDL2 takes a button's position from the last mousemove: send one first
	   (touch taps and some synthetic clicks have none) */
	var mx = -1, my = -1;
	['mousedown', 'mouseup'].forEach(function (t) {
		$('canvas').addEventListener(t, function (e) {
			if (e.clientX === mx && e.clientY === my) return;
			this.dispatchEvent(new MouseEvent('mousemove', { clientX: e.clientX, clientY: e.clientY, buttons: e.buttons, bubbles: true }));
		}, true);
	});
	$('canvas').addEventListener('mousemove', function (e) { mx = e.clientX; my = e.clientY; }, true);
	document.addEventListener('DOMContentLoaded', function () {
		$('btn-help').onclick = function () { $('help').hidden ? showHelp() : closeHelp(); };
		/* the second document: the original manual, reached from Help (head button or the guide's Manual section) */
		$('help-switch').onclick = function () { $('help-frame').hidden ? showHelp(0) : showHelp(); };
		$('help-body').addEventListener('click', function (e) {
			var a = e.target.closest && e.target.closest('a[data-manual]');
			if (a) { e.preventDefault(); showHelp(0); }
		});
		$('help-close').onclick = closeHelp;
		/* Escape inside the manual (same-origin iframe) closes it too */
		$('help-frame').onload = function () {
			try { this.contentWindow.addEventListener('keydown', function (e) { if (e.key === 'Escape') closeHelp(); }); } catch (e) { }
		};
		RvipWM.dropdown($('btn-file'), $('menu-file'));
		RvipWM.dropdown($('btn-audio'), $('menu-audio'));
		$('chk-sound').onchange = function () { setSound(this.checked, true); $('canvas').focus(); };
		$('btn-restart').onclick = function () { location.reload(); };
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
	});
})();
