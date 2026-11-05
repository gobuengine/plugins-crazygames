# Tests

`python run_tests.py`: the plugin's real web library in a real web game. `projects/crazygames_game` is exported
for the web with the plugin (and Authentication, next to this repository), and its page gets
`fake_crazygames_sdk.js` in place of the CrazyGames SDK. Headless Chrome plays it twice:

- **With the SDK:** Services takes the `crazygames` provider. The game gets a midgame ad (and, after it, the
  player's mute setting mutes the game), a rewarded ad that gives the reward, an invite link and its parameter,
  the player, and a sign-in through Authentication (`crazygames:<token>`). The SDK is asked in order: loading,
  gameplay start, stop before each ad, start after it, never two of a kind in a row.
- **Without it** (blocked): the game still runs to its end, without ads, links or player.

Needs Python 3, Chrome (or `CHROME=<path>`) and Gobu built next to this repository (`ProjectGobu/Gobu`) with
its web template without threads (`forja build TemplateWeb_r2d_nothreads`). Build the plugin first
(`forja build PluginCrazyGames` in Gobu). What a run leaves is in `.run/`.
