/**************************************************************************/
/*  crazygames_web.h                                                      */
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

// What the plugin puts in the page (JavaScriptBridge.eval): window.gobuCrazyGames, over the CrazyGames SDK v3 the
// page loaded (plugin.json "web_head"). The SDK answers with promises and callbacks; the plugin gets each answer
// through a callback (JavaScriptBridge.create_callback). Without the SDK, or on another site ("disabled", where
// every call throws), everything answers "no".
static const char *CRAZYGAMES_WEB_SCRIPT = R"(
(function () {
	if (window.gobuCrazyGames) {
		return;
	}
	const state = { ready: false };
	let initialized = null; // SDK.init()'s promise: what is asked before it ends waits for it, in order.
	const sdk = function () {
		return (window.CrazyGames && window.CrazyGames.SDK) ? window.CrazyGames.SDK : null;
	};
	const safely = function (work, otherwise) {
		try {
			return work();
		} catch (error) {
			return otherwise;
		}
	};
	const after = function (work) {
		(initialized || Promise.resolve()).then(function () {
			safely(work);
		});
	};
	window.gobuCrazyGames = {
		init: function (done, settingsChanged, userChanged) {
			const crazy = sdk();
			if (!crazy) {
				initialized = Promise.resolve();
				done(false, '', false, false);
				return;
			}
			const starting = safely(function () { return crazy.init(); }, null);
			initialized = (starting ? Promise.resolve(starting) : Promise.reject(new Error('CrazyGames.SDK.init'))).then(function () {
				const environment = String(safely(function () { return crazy.environment; }, 'disabled') || 'disabled');
				if (environment === 'disabled') {
					done(false, environment, false, false);
					return;
				}
				state.ready = true;
				safely(function () { crazy.game.loadingStart(); });
				safely(function () {
					crazy.game.addSettingsChangeListener(function (settings) {
						settingsChanged(!!(settings && settings.muteAudio));
					});
				});
				safely(function () {
					crazy.user.addAuthListener(function () {
						userChanged();
					});
				});
				const muted = !!safely(function () { return crazy.game.settings.muteAudio; }, false);
				return Promise.resolve(safely(function () { return crazy.ad.hasAdblock(); }, false)).then(function (blocked) {
					done(true, environment, !!blocked, muted);
				}, function () {
					done(true, environment, false, muted);
				});
			}, function () {
				done(false, 'disabled', false, false);
			});
		},
		loadingFinished: function () {
			after(function () { if (state.ready) { sdk().game.loadingStop(); } });
		},
		gameplayStart: function () {
			after(function () { if (state.ready) { sdk().game.gameplayStart(); } });
		},
		gameplayStop: function () {
			after(function () { if (state.ready) { sdk().game.gameplayStop(); } });
		},
		// "midgame" or "rewarded": done(true) when the ad played to its end (for a rewarded one: give the reward).
		requestAd: function (kind, started, done) {
			after(function () {
				if (!state.ready) {
					done(false);
					return;
				}
				let began = false;
				let ended = false;
				const end = function (shown) {
					if (!ended) {
						ended = true;
						done(shown);
					}
				};
				if (safely(function () {
					sdk().ad.requestAd(kind, {
						adStarted: function () {
							began = true;
							started();
						},
						adFinished: function () {
							end(began || kind === 'rewarded');
						},
						adError: function () {
							end(false);
						},
					});
					return true;
				}, false) !== true) {
					end(false);
				}
			});
		},
		inviteLink: function (params, done) {
			after(function () {
				if (!state.ready) {
					done('');
					return;
				}
				Promise.resolve(safely(function () { return sdk().game.inviteLink(JSON.parse(params)); }, '')).then(function (url) {
					done(url ? String(url) : '');
				}, function () {
					done('');
				});
			});
		},
		getInviteParam: function (name) {
			if (state.ready) {
				return String(safely(function () { return sdk().game.getInviteParam(name); }, '') || '');
			}
			return new URLSearchParams(window.location.search).get(name) || '';
		},
		getUser: function (done) {
			after(function () {
				if (!state.ready || !safely(function () { return sdk().user.isUserAccountAvailable; }, false)) {
					done('', '');
					return;
				}
				Promise.resolve(sdk().user.getUser()).then(function (user) {
					done(user && user.username ? String(user.username) : '', user && user.profilePictureUrl ? String(user.profilePictureUrl) : '');
				}, function () {
					done('', '');
				});
			});
		},
		showAuthPrompt: function () {
			if (state.ready) {
				Promise.resolve(safely(function () { return sdk().user.showAuthPrompt(); }, null)).catch(function () {});
			}
		},
		happytime: function () {
			after(function () { if (state.ready) { sdk().game.happytime(); } });
		},
		reportCompleted: function (percent) {
			after(function () { if (state.ready) { sdk().game.reportGameCompletedPercentage(percent); } });
		},
	};
})();
)";
