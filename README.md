# CrazyGames

Publish a Gobu web game on [CrazyGames](https://www.crazygames.com), as a Gobu plugin.

The game talks to CrazyGames through Gobu's **Services**, the same API every game service uses: the plugin is
its `crazygames` provider. Ads (midgame and rewarded), game events, invite links and the player go there, so the
same game goes to Poki or to CrazyGames by enabling one plugin or the other:

```grost
async fn on_revive_pressed() {
	if await Services.rewarded_break() {   # A CrazyGames rewarded ad: true when the player saw it to the end.
		revive()
	}
}
```

Services also sends the game events by itself and mutes, pauses and ignores the input of the game during an
ad. The plugin follows the player's mute setting on CrazyGames (`muteAudio`). The `CrazyGames` singleton has
what only CrazyGames has: `happytime()`, `report_game_completed()`, `show_auth_prompt()`, `is_ad_blocked()`,
`get_environment()`, `is_ready()`. Signing in for an online game is the Authentication plugin's
(`Authentication.sign_in_with("crazygames")`).

- `crazygames/`: the plugin as a project has it: its `plugin.json` (its `web_head` puts the CrazyGames SDK v3
  in the exported page's `<head>`), `doc_classes/CrazyGames.xml`, `icon.svg` and its libraries in `bin/`.
- `Source/`: its code, C++ against Gobu's plugin interface only (`gobu_plugin.h`, version 4): a WebAssembly side
  module for the web templates without threads (through Gobu's JavaScriptBridge), and a Windows library with
  only the singleton, for the editor. `forja build PluginCrazyGames` in Gobu (this repository in
  `ProjectGobu/Plugins/CrazyGames`, listed in Gobu's `Engine/Source/external_plugins.txt`).
- `Tests/`: `python run_tests.py` exports a test game for the web and plays it in headless Chrome against a
  fake CrazyGames SDK (`Tests/fake_crazygames_sdk.js`), with and without it.

The SDK works on CrazyGames and on localhost (test ads); elsewhere it is disabled. Guide:
https://gobuengine.org/plugins/04-crazygames
