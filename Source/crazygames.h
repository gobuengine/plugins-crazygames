/**************************************************************************/
/*  crazygames.h                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GOBU ENGINE                               */
/*                        https://gobuengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Gobu Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "gobu.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// CrazyGames (crazygames.com) for Gobu's Services: in a web game, its SDK (the page loads it, plugin.json
// "web_head") is the "crazygames" provider of Services (ads, game events, invite links, the player), and the
// `CrazyGames` singleton has what only CrazyGames has (happy time, the completion, its sign-in prompt). The
// game's sound follows the player's mute setting on CrazyGames by itself. On other platforms the singleton is
// there (the editor knows `CrazyGames.*`) and does nothing.
class CrazyGames {
public:
	static constexpr const char *CLASS_NAME = "CrazyGames";
	static constexpr const char *PROVIDER_NAME = "crazygames";

	static CrazyGames *singleton;
	GobuObject *self = nullptr; // Its object: the singleton.

	// The plugin's class: its methods (gobu_plugin_init()).
	static bool register_class();
	// The provider of Services (web only).
	static bool register_provider();

	explicit CrazyGames(GobuObject *p_self);
	~CrazyGames();

	// The SDK loaded and started on CrazyGames (or on localhost, where it shows test ads). False outside a web
	// game, on another site ("disabled") or before it started.
	bool is_ready() const { return ready; }
	// "crazygames", "local" (localhost: test ads), "disabled" (another site: nothing works) or "" (no SDK).
	std::string get_environment() const { return environment; }
	bool is_ad_blocked() const { return ad_blocked; }
	// A moment of joy (a level beaten, a record): CrazyGames celebrates it. Sparingly.
	void happytime();
	// How much of the game the player has done, 0 to 100.
	void report_game_completed(int64_t p_percent);
	// CrazyGames's sign-in prompt (on a player's press); the player comes through Services.
	void show_auth_prompt();

	// Once a frame (the plugin's process callback): starts the SDK, frees the callbacks of what answered.
	void frame() {
		start_sdk();
		retired.clear();
	}
	// Puts the plugin's script in the page and starts the SDK (once: the first frame, or when Services takes this
	// provider before it). Nothing outside a web game.
	void start_sdk();

private:
	bool ready = false;
	bool ad_blocked = false;
	bool started = false;
	bool muted_by_settings = false;
	std::string environment;
	std::string user_name;
	std::string user_avatar_url;
	const GobuServicesEvents *events = nullptr;
	gobu::Value bridge; // window.gobuCrazyGames (a JavaScriptObject).
	std::map<int64_t, std::vector<gobu::Value>> waiting; // The JavaScript callbacks of what waits, by request.
	std::vector<gobu::Value> kept; // Callbacks that live as long as the game (the start, the listeners).
	std::vector<gobu::Value> retired; // Of what answered: freed on the next frame, not inside JavaScript's call.

	gobu::Value _js_callback(std::function<void(const gobu::Args &)> p_function);
	static gobu::Value _arg(const gobu::Args &p_args, int p_index);
	gobu::Value _bridge_call(const char *p_method, std::initializer_list<gobu::Value> p_args = {});
	void _read_player();
	void _mute(bool p_muted);
	void _break(int64_t p_request, bool p_rewarded);
	void _answered(int64_t p_request);

	// The provider's functions (GobuServicesProviderInfo).
	static void _init(void *p_userdata, const GobuServicesEvents *p_events);
	static void _shutdown(void *p_userdata);
	static void _loading_finished(void *p_userdata);
	static void _gameplay_start(void *p_userdata);
	static void _gameplay_stop(void *p_userdata);
	static void _commercial_break(void *p_userdata, int64_t p_request);
	static void _rewarded_break(void *p_userdata, int64_t p_request);
	static void _shareable_url(void *p_userdata, int64_t p_request, const GobuVariant *p_params);
	static void _get_url_param(void *p_userdata, const char *p_name, GobuVariant *r_value);
	static bool _get_player(void *p_userdata, GobuVariant *r_name, GobuVariant *r_avatar_url);
};
