# The CrazyGames plugin in a real web game: projects/crazygames_game is exported for the web with the plugin, its
# page gets the fake SDK of fake_crazygames_sdk.js in place of CrazyGames's (which only answers on crazygames.com and
# localhost), and headless Chrome plays it. The game asks Services for breaks, an invite link, its parameter and the
# player; the runner checks what the game got ("CG_TEST" lines) and what the SDK was asked, in order ("CG_SDK").
#
#   python run_tests.py [gobu...console.exe]
#
# Needs Gobu built next to this repository (ProjectGobu/Gobu) with its web template without threads, Chrome, and
# the plugin built (`forja build PluginCrazyGames` in Gobu).
import functools
import http.server
import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
GOBU = os.path.normpath(os.path.join(HERE, '..', '..', '..', 'Gobu'))
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(GOBU, 'Engine', 'Binaries', 'gobu.windows.editor.x86_64.console.exe')
CHROME = os.environ.get('CHROME', 'C:/Program Files/Google/Chrome/Application/chrome.exe')
PROJECT = os.path.join(HERE, 'projects', 'crazygames_game')
RUN = os.path.join(HERE, '.run')
OUT = os.path.join(RUN, 'web')
PORT = 8072
SDK = 'https://sdk.crazygames.com/crazygames-sdk-v3.js'

failures = []


def check(what, ok, detail=''):
	print(('  ok   ' if ok else '  FAIL ') + what + ('' if ok or not detail else ': ' + detail))
	if not ok:
		failures.append(what)


def export():
	if os.path.exists(RUN):
		shutil.rmtree(RUN)
	os.makedirs(OUT)
	plugin = os.path.join(PROJECT, 'plugins', 'crazygames')
	if os.path.exists(plugin):
		shutil.rmtree(plugin)
	shutil.copytree(os.path.join(HERE, '..', 'crazygames'), plugin)
	# Authentication (next to this repository), for CrazyGames's sign-in.
	authentication = os.path.join(HERE, '..', '..', 'Authentication', 'authentication')
	plugin = os.path.join(PROJECT, 'plugins', 'authentication')
	if os.path.exists(plugin):
		shutil.rmtree(plugin)
	if os.path.exists(authentication):
		shutil.copytree(authentication, plugin)
	appdata = os.path.join(RUN, 'appdata')
	os.makedirs(appdata)
	env = dict(os.environ, APPDATA=appdata, LOCALAPPDATA=appdata)
	# The project's first import, then the export.
	subprocess.run([EXE, '--headless', '--path', PROJECT, '--editor', '--quit'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
	result = subprocess.run([EXE, '--headless', '--path', PROJECT, '--export-release', 'Web', os.path.join(OUT, 'index.html')], env=env, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=1200)
	open(os.path.join(RUN, 'export.log'), 'w', encoding='utf-8').write(result.stdout + result.stderr)
	page = os.path.join(OUT, 'index.html')
	if not os.path.exists(page):
		print(result.stdout[-2000:], result.stderr[-2000:])
		raise SystemExit('The export made no page: see .run/export.log.')
	html = open(page, encoding='utf-8').read()
	check('the page loads the CrazyGames SDK in its <head> (plugin.json "web_head")', '<script src="%s"></script>' % SDK in html.split('</head>')[0])
	check('the plugin\'s web library is in the export', any(name.startswith('libcrazygames') for name in os.listdir(OUT)), ', '.join(os.listdir(OUT)))
	# The fake SDK in its place, and the same page with the SDK blocked (as an ad blocker does).
	shutil.copy(os.path.join(HERE, 'fake_crazygames_sdk.js'), os.path.join(OUT, 'fake_crazygames_sdk.js'))
	open(page, 'w', encoding='utf-8').write(html.replace(SDK, 'fake_crazygames_sdk.js'))
	open(os.path.join(OUT, 'blocked.html'), 'w', encoding='utf-8').write(html.replace(SDK, 'blocked/crazygames-sdk-v3.js'))


class QuietHandler(http.server.SimpleHTTPRequestHandler):
	def log_message(self, *args):
		pass


def play(page='index.html'):
	handler = functools.partial(QuietHandler, directory=OUT)
	server = http.server.ThreadingHTTPServer(('127.0.0.1', PORT), handler)
	threading.Thread(target=server.serve_forever, daemon=True).start()
	name = page.split('.')[0]
	profile = os.path.join(RUN, 'chrome_' + name)
	log_path = os.path.join(RUN, 'chrome_%s.log' % name)
	log = open(log_path, 'w', encoding='utf-8')
	chrome = subprocess.Popen([CHROME, '--headless=new', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--enable-logging=stderr', '--v=0', '--no-first-run', '--autoplay-policy=no-user-gesture-required', '--user-data-dir=' + profile, 'http://127.0.0.1:%d/%s' % (PORT, page)], stdout=log, stderr=subprocess.STDOUT)
	deadline = time.time() + 90
	text = ''
	while time.time() < deadline:
		time.sleep(1)
		text = open(log_path, encoding='utf-8', errors='replace').read()
		if 'CG_TEST done' in text:
			time.sleep(1)
			text = open(log_path, encoding='utf-8', errors='replace').read()
			break
	chrome.kill()
	chrome.wait()
	server.shutdown()
	server.server_close()
	return text


def console(text, prefix):
	lines = []
	# Chrome writes them as `INFO:CONSOLE:<line>] "<text>", source: <file> (<line>)`.
	for match in re.finditer(r'CONSOLE[:(]\d+\)?\] "(.*)", source:', text):
		line = match.group(1)
		if line.startswith(prefix + ' '):
			lines.append(line[len(prefix) + 1:])
	return lines


def errors_in(text):
	return [line for line in text.splitlines() if 'Uncaught' in line or 'SCRIPT ERROR' in line]


def main():
	print('Export')
	export()
	print('Play')
	text = play()
	game = console(text, 'CG_TEST')
	sdk = console(text, 'CG_SDK')
	check('the game ran to its end', 'done' in game, ' | '.join(game) or 'no CG_TEST line (see .run/chrome_index.log)')
	expected = {
		'provider=crazygames': 'Services takes the plugin\'s provider',
		'commercial=true muted=true': 'a midgame ad plays, and the player\'s mute setting mutes the game',
		'rewarded=true': 'a rewarded ad gives the reward',
		'url=https://www.crazygames.com/game/test-game?room=r42': 'shareable_url gives the invite link',
		'param=r42': 'get_url_param reads its parameter',
		'ready=true env=crazygames player=user1': 'the SDK is ready on CrazyGames and the player is its',
		'signed_in=crazygames:fake-cg-token username=user1': 'Authentication signs in with CrazyGames (its token, for a server to check)',
	}
	for line, what in expected.items():
		check(what, line in game, ' | '.join(game))
	# What the SDK was asked, in order (the start's own calls and the player go when it is ready).
	calls = [call for call in sdk if call not in ('init', 'hasAdblock', 'getUser') and not call.startswith('getUserToken')]
	order = ['loadingStart', 'loadingStop', 'gameplayStart', 'gameplayStop', 'requestAd midgame', 'gameplayStart', 'gameplayStop', 'requestAd rewarded', 'gameplayStart', 'inviteLink {"room":"r42"}', 'getInviteParam room', 'happytime', 'reportGameCompletedPercentage 50']
	check('the SDK is asked in order (loading, then never two starts in a row, stop before an ad, start after it)', calls[:len(order)] == order, ' | '.join(calls))
	check('the player is read from the SDK', 'getUser' in sdk)
	check('the sign-in asks for an IN_GAME_AUTH token', 'getUserToken IN_GAME_AUTH' in sdk)
	check('no JavaScript or script errors', not errors_in(text), ' | '.join(errors_in(text)[:5]))

	print('Play without the SDK (blocked)')
	text = play('blocked.html')
	game = console(text, 'CG_TEST')
	check('the game still runs to its end', 'done' in game, ' | '.join(game) or 'no CG_TEST line (see .run/chrome_blocked.log)')
	for line, what in {
		'commercial=false muted=false': 'a break ends without an ad (and nothing mutes the game)',
		'rewarded=false': 'a rewarded break gives no reward',
		'url=': 'no invite link',
		'ready=false env= player=': 'the SDK is not ready, and there is no player',
	}.items():
		check(what, line in game, ' | '.join(game))
	check('no JavaScript or script errors without it', not errors_in(text), ' | '.join(errors_in(text)[:5]))
	print('%d failed' % len(failures) if failures else 'All passed')
	return 1 if failures else 0


if __name__ == '__main__':
	sys.exit(main())
