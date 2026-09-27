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
	var running = false, sound = false;

	function $(id) { return document.getElementById(id); }
	function status(msg, isError) {
		var s = $('status');
		s.textContent = msg; s.hidden = !msg; s.classList.toggle('error', !!isError);
	}

	/* ---------- scaling: largest size that fits, whole pixels when it can ---------- */
	function fit() {
		var g = $('game'), cv = $('canvas');
		var s = Math.min(g.clientWidth / 640, g.clientHeight / 480);
		if (s >= 1) s = Math.floor(s * 4) / 4;   /* quarter steps, like the Mac window */
		cv.style.width = Math.floor(640 * s) + 'px';
		cv.style.height = Math.floor(480 * s) + 'px';
	}

	/* ---------- saves: IndexedDB (IDBFS) ---------- */
	var syncing = false, syncAgain = false;
	function syncFiles() {
		if (!Module.FS) return;
		if (syncing) { syncAgain = true; return; }
		syncing = true;
		Module.FS.syncfs(false, function (err) {
			syncing = false;
			if (err) status('Saving to browser storage (IndexedDB) failed: ' + err + '. Use "Export save" to keep a copy.', true);
			if (syncAgain) { syncAgain = false; syncFiles(); }
		});
	}
	function saves() {
		try {
			return Module.FS.readdir(DIR).filter(function (n) { return /\.dsg$/i.test(n); })
				.map(function (n) { return { name: n, t: Module.FS.stat(DIR + '/' + n).mtime.getTime() }; })
				.sort(function (a, b) { return b.t - a.t; });
		} catch (e) { return []; }
	}
	function exportSave() {
		var s = saves()[0];
		if (!s) { status('There is no saved game yet (Options → Save As…).', true); setTimeout(function () { status(''); }, 2500); return; }
		var a = document.createElement('a');
		a.href = URL.createObjectURL(new Blob([Module.FS.readFile(DIR + '/' + s.name)], { type: 'application/octet-stream' }));
		a.download = s.name;
		document.body.appendChild(a); a.click();
		setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
	}
	function importSave(file) {
		var r = new FileReader();
		r.onload = function () {
			var name = file.name.replace(/[^\w.-]/g, '_');
			if (!/\.dsg$/i.test(name)) name += '.DSG';
			Module.FS.writeFile(DIR + '/' + name, new Uint8Array(r.result));
			syncFiles();
			status('Imported ' + name + '. Load it with Options → Load Game (or Load on the title screen).');
			setTimeout(function () { status(''); }, 4000);
		};
		r.readAsArrayBuffer(file);
	}

	/* ---------- sound (off by default, remembered in IndexedDB) ---------- */
	function setSound(on, store) {
		sound = on;
		$('chk-sound').checked = on;
		if (running) Module._web_set_sound(on ? 1 : 0);
		if (store) { try { Module.FS.writeFile(SETTINGS, JSON.stringify({ sound: on })); syncFiles(); } catch (e) { } }
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
			body.hidden = true; fr.hidden = false;
			fr.src = 'doc/index.html' + (manualCtx ? '#h' + manualCtx : '');
			fr.focus();
			return;
		}
		$('help-title').textContent = 'Decker guide';
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
	window.deckerSync = syncFiles;
	window.deckerEnd = function () {
		running = false;
		syncFiles();
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
	window.Module = {
		canvas: document.getElementById('canvas'),
		preRun: [function () {
			var FS = Module.FS;
			FS.mkdirTree(DIR);
			FS.mount(Module.IDBFS, {}, DIR);
			Module.addRunDependency('idbfs');
			FS.syncfs(true, function (err) {
				if (err) status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				try { sound = !!JSON.parse(FS.readFile(SETTINGS, { encoding: 'utf8' })).sound; } catch (e) { }
				Module.removeRunDependency('idbfs');
			});
		}],
		onRuntimeInitialized: function () {
			running = true; status('');
			$('game').hidden = false;
			setTimeout(function () { fit(); setSound(sound, false); $('canvas').focus(); }, 0);
		},
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !running) status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { crashed(what); }
	};
	function crashed(err) {
		if (!running) return;
		running = false;
		var msg = (err && (err.message || err.reason && err.reason.message)) || String(err);
		console.error('[decker] crash:', err);
		status('The game crashed (' + msg + '). Reload the page; your saves are kept.', true);
	}
	window.addEventListener('unhandledrejection', function (e) {
		if (e.reason && e.reason.name === 'ExitStatus') return;
		crashed(e.reason);
	});
	window.addEventListener('error', function (e) {
		if (e.error && e.error.name === 'ExitStatus') return;
		if (e.error instanceof WebAssembly.RuntimeError || /decker-core/.test(e.filename || '')) crashed(e.error || e.message);
	});
	window.addEventListener('beforeunload', function (e) { if (running) { e.preventDefault(); e.returnValue = ''; } });
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
		$('btn-export').onclick = exportSave;
		$('btn-import').onclick = function () { $('import-file').click(); };
		$('import-file').onchange = function () { if (this.files[0]) importSave(this.files[0]); this.value = ''; };
		$('btn-help').onclick = function () { $('help').hidden ? showHelp() : closeHelp(); };
		$('btn-manual').onclick = function () { showHelp(0); };
		$('help-close').onclick = closeHelp;
		/* Escape inside the manual (same-origin iframe) closes it too */
		$('help-frame').onload = function () {
			try { this.contentWindow.addEventListener('keydown', function (e) { if (e.key === 'Escape') closeHelp(); }); } catch (e) { }
		};
		RvipWM.dropdown($('btn-file'), $('menu-file'));
		RvipWM.dropdown($('btn-audio'), $('menu-audio'));
		$('chk-sound').onchange = function () { setSound(this.checked, true); $('canvas').focus(); };
		$('btn-new').onclick = function () { location.reload(); };
		$('btn-restart').onclick = function () { location.reload(); };
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
	});
})();
