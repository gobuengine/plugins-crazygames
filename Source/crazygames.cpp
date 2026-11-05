/**************************************************************************/
/*  crazygames.cpp                                                        */
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

#include "crazygames.h"

#include "crazygames_web.h"

using gobu::api;
using gobu::Value;

CrazyGames *CrazyGames::singleton = nullptr;

namespace {

CrazyGames *_of(void *p_instance) {
	return static_cast<CrazyGames *>(p_instance);
}

void _return(GobuVariant *r_return, const Value &p_value) {
	api->variant_destroy(r_return);
	api->variant_new_copy(r_return, p_value.raw());
}

void _is_ready(void *, void *p_instance, const GobuVariant *const *, int64_t, GobuVariant *r_return, GobuCallError *) {
	_return(r_return, Value(_of(p_instance)->is_ready()));
}

void _get_environment(void *, void *p_instance, const GobuVariant *const *, int64_t, GobuVariant *r_return, GobuCallError *) {
	_return(r_return, Value(_of(p_instance)->get_environment()));
}

void _is_ad_blocked(void *, void *p_instance, const GobuVariant *const *, int64_t, GobuVariant *r_return, GobuCallError *) {
	_return(r_return, Value(_of(p_instance)->is_ad_blocked()));
}

void _happytime(void *, void *p_instance, const GobuVariant *const *, int64_t, GobuVariant *, GobuCallError *) {
	_of(p_instance)->happytime();
}

void _report_game_completed(void *, void *p_instance, const GobuVariant *const *p_args, int64_t, GobuVariant *, GobuCallError *) {
	_of(p_instance)->report_game_completed(Value::copy_of(p_args[0]).to_int());
}

void _show_auth_prompt(void *, void *p_instance, const GobuVariant *const *, int64_t, GobuVariant *, GobuCallError *) {
	_of(p_instance)->show_auth_prompt();
}

void *_create(void *, GobuObject *p_object) {
	return new CrazyGames(p_object);
}

void _free(void *, void *p_instance) {
	delete _of(p_instance);
}

void _method(const char *p_name, GobuMethodFunc p_func, std::initializer_list<GobuArgInfo> p_args = {}, int p_return_type = -1) {
	const std::vector<GobuArgInfo> args(p_args);
	GobuMethodInfo info = {};
	info.name = p_name;
	info.func = p_func;
	info.has_return = p_return_type >= 0;
	info.return_info = { "", p_return_type >= 0 ? p_return_type : GOBU_TYPE_NIL, 0, nullptr };
	info.arg_count = int32_t(args.size());
	info.args = args.data();
	api->register_method(gobu::plugin, CrazyGames::CLASS_NAME, &info);
}

} // namespace

bool CrazyGames::register_class() {
	GobuClassInfo info = {};
	info.name = CLASS_NAME;
	info.parent = "Object";
	info.create_instance = _create;
	info.free_instance = _free;
	if (!api->register_class(gobu::plugin, &info)) {
		return false;
	}
	_method("is_ready", _is_ready, {}, GOBU_TYPE_BOOL);
	_method("get_environment", _get_environment, {}, GOBU_TYPE_STRING);
	_method("is_ad_blocked", _is_ad_blocked, {}, GOBU_TYPE_BOOL);
	_method("happytime", _happytime);
	_method("report_game_completed", _report_game_completed, { { "percent", GOBU_TYPE_INT, 0, nullptr } });
	_method("show_auth_prompt", _show_auth_prompt);
	return true;
}

bool CrazyGames::register_provider() {
	GobuServicesProviderInfo info = {};
	info.size = sizeof(info);
	info.name = PROVIDER_NAME;
	info.features = GOBU_SERVICES_ADS | GOBU_SERVICES_REWARDED_ADS | GOBU_SERVICES_GAME_EVENTS | GOBU_SERVICES_SHARE | GOBU_SERVICES_PLAYER;
	info.init = _init;
	info.shutdown = _shutdown;
	info.loading_finished = _loading_finished;
	info.gameplay_start = _gameplay_start;
	info.gameplay_stop = _gameplay_stop;
	info.commercial_break = _commercial_break;
	info.rewarded_break = _rewarded_break;
	info.shareable_url = _shareable_url;
	info.get_url_param = _get_url_param;
	info.get_player = _get_player;
	return api->register_services_provider(gobu::plugin, &info);
}

CrazyGames::CrazyGames(GobuObject *p_self) :
		self(p_self) {
	singleton = this;
}

CrazyGames::~CrazyGames() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

// The page's JavaScript.

Value CrazyGames::_js_callback(std::function<void(const gobu::Args &)> p_function) {
	GobuObject *js = gobu::singleton("JavaScriptBridge");
	return js ? gobu::call(js, "create_callback", { gobu::callable(std::move(p_function), self) }) : Value();
}

// create_callback() calls back with one Array, of the JavaScript arguments.
Value CrazyGames::_arg(const gobu::Args &p_args, int p_index) {
	if (p_args.empty() || p_args[0].size() <= p_index) {
		return Value();
	}
	return p_args[0].at(p_index);
}

Value CrazyGames::_bridge_call(const char *p_method, std::initializer_list<Value> p_args) {
	if (bridge.object() == nullptr) {
		return Value();
	}
	return gobu::call(bridge.object(), p_method, p_args);
}

void CrazyGames::start_sdk() {
	if (started) {
		return;
	}
	started = true;
	GobuObject *js = gobu::singleton("JavaScriptBridge");
	if (!js) {
		return; // Not a web game: nothing of CrazyGames here.
	}
	gobu::call(js, "eval", { CRAZYGAMES_WEB_SCRIPT, true });
	bridge = gobu::call(js, "get_interface", { "gobuCrazyGames" });
	if (bridge.object() == nullptr) {
		GOBU_WARNING("CrazyGames: the page could not run the plugin's script: the game goes on without CrazyGames.");
		return;
	}
	const Value done = _js_callback([this](const gobu::Args &p_args) {
		ready = _arg(p_args, 0).to_bool();
		environment = _arg(p_args, 1).str();
		ad_blocked = _arg(p_args, 2).to_bool();
		if (!ready) {
			GOBU_VERBOSE(environment.empty() ? "CrazyGames: the page has no CrazyGames SDK (plugin.json \"web_head\")." : "CrazyGames: the SDK is \"" + environment + "\" here (it works on CrazyGames and on localhost).");
			return;
		}
		_mute(_arg(p_args, 3).to_bool());
		_read_player();
	});
	// The player's mute setting on CrazyGames wins over the game's sound.
	const Value settings_changed = _js_callback([this](const gobu::Args &p_args) {
		_mute(_arg(p_args, 0).to_bool());
	});
	const Value user_changed = _js_callback([this](const gobu::Args &) {
		_read_player();
	});
	kept.insert(kept.end(), { done, settings_changed, user_changed });
	_bridge_call("init", { done, settings_changed, user_changed });
}

void CrazyGames::_mute(bool p_muted) {
	GobuObject *audio = gobu::singleton("Audio");
	if (!audio || p_muted == muted_by_settings) {
		return;
	}
	muted_by_settings = p_muted;
	gobu::call(audio, "set_bus_mute", { 0, p_muted });
}

void CrazyGames::_read_player() {
	const Value done = _js_callback([this](const gobu::Args &p_args) {
		const std::string name = _arg(p_args, 0).str();
		const std::string avatar = _arg(p_args, 1).str();
		const bool changed = name != user_name || avatar != user_avatar_url;
		user_name = name;
		user_avatar_url = avatar;
		if (changed && events) {
			events->player_changed(events->services);
		}
	});
	kept.push_back(done);
	_bridge_call("getUser", { done });
}

void CrazyGames::_answered(int64_t p_request) {
	std::map<int64_t, std::vector<Value>>::iterator found = waiting.find(p_request);
	if (found != waiting.end()) {
		retired.insert(retired.end(), found->second.begin(), found->second.end());
		waiting.erase(found);
	}
}

void CrazyGames::_break(int64_t p_request, bool p_rewarded) {
	if (bridge.object() == nullptr || !events) {
		if (events) {
			events->ad_finished(events->services, p_request, false); // No SDK: no ad, the game goes on.
		}
		return;
	}
	const Value started_callback = _js_callback([this, p_request](const gobu::Args &) {
		if (events) {
			events->ad_started(events->services, p_request);
		}
	});
	const Value done_callback = _js_callback([this, p_request](const gobu::Args &p_args) {
		if (events) {
			events->ad_finished(events->services, p_request, _arg(p_args, 0).to_bool());
		}
		_answered(p_request);
	});
	waiting[p_request] = { started_callback, done_callback };
	_bridge_call("requestAd", { p_rewarded ? "rewarded" : "midgame", started_callback, done_callback });
}

void CrazyGames::happytime() {
	_bridge_call("happytime");
}

void CrazyGames::report_game_completed(int64_t p_percent) {
	_bridge_call("reportCompleted", { std::max<int64_t>(0, std::min<int64_t>(100, p_percent)) });
}

void CrazyGames::show_auth_prompt() {
	_bridge_call("showAuthPrompt");
}

// The provider of Services.

void CrazyGames::_init(void *, const GobuServicesEvents *p_events) {
	if (singleton) {
		singleton->events = p_events;
		singleton->start_sdk();
	}
}

void CrazyGames::_shutdown(void *) {
	if (singleton) {
		singleton->events = nullptr;
		singleton->waiting.clear();
	}
}

void CrazyGames::_loading_finished(void *) {
	if (singleton) {
		singleton->_bridge_call("loadingFinished");
	}
}

void CrazyGames::_gameplay_start(void *) {
	if (singleton) {
		singleton->_bridge_call("gameplayStart");
	}
}

void CrazyGames::_gameplay_stop(void *) {
	if (singleton) {
		singleton->_bridge_call("gameplayStop");
	}
}

void CrazyGames::_commercial_break(void *, int64_t p_request) {
	if (singleton) {
		singleton->_break(p_request, false);
	}
}

void CrazyGames::_rewarded_break(void *, int64_t p_request) {
	if (singleton) {
		singleton->_break(p_request, true);
	}
}

// CrazyGames's invite link: a link to this game with the parameters, read back with getInviteParam().
void CrazyGames::_shareable_url(void *, int64_t p_request, const GobuVariant *p_params) {
	CrazyGames *crazy = singleton;
	if (!crazy || !crazy->events) {
		return;
	}
	if (crazy->bridge.object() == nullptr) {
		crazy->events->url_ready(crazy->events->services, p_request, "");
		return;
	}
	// JavaScript takes text: the parameters go as JSON.
	const std::string params = gobu::instantiate("JSON").call("stringify", { Value::copy_of(p_params) }).str();
	const Value done = crazy->_js_callback([crazy, p_request](const gobu::Args &p_args) {
		if (crazy->events) {
			crazy->events->url_ready(crazy->events->services, p_request, _arg(p_args, 0).str().c_str());
		}
		crazy->_answered(p_request);
	});
	crazy->waiting[p_request] = { done };
	crazy->_bridge_call("inviteLink", { params, done });
}

void CrazyGames::_get_url_param(void *, const char *p_name, GobuVariant *r_value) {
	if (!singleton || singleton->bridge.object() == nullptr) {
		return;
	}
	const std::string value = singleton->_bridge_call("getInviteParam", { p_name }).str();
	if (!value.empty()) {
		api->variant_new_string(r_value, value.c_str(), int64_t(value.size()));
	}
}

bool CrazyGames::_get_player(void *, GobuVariant *r_name, GobuVariant *r_avatar_url) {
	if (!singleton || singleton->user_name.empty()) {
		return false;
	}
	api->variant_new_string(r_name, singleton->user_name.c_str(), int64_t(singleton->user_name.size()));
	api->variant_new_string(r_avatar_url, singleton->user_avatar_url.c_str(), int64_t(singleton->user_avatar_url.size()));
	return true;
}
