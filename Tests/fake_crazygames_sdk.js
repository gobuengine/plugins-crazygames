// A CrazyGames SDK v3 for the tests, in place of sdk.crazygames.com's (run_tests.py swaps it into the exported
// page): it answers as the real one does on CrazyGames and writes each call to the console ("CG_SDK <call>"), in
// order, for the runner. The player's mute setting turns on after the first ad, as a player would.
(function () {
	const log = function (text) {
		console.log('CG_SDK ' + text);
	};
	const later = function (ms, value) {
		return new Promise(function (resolve) {
			setTimeout(function () { resolve(value); }, ms);
		});
	};
	const settingsListeners = [];
	const settings = { muteAudio: false, disableChat: false };
	window.CrazyGames = {
		SDK: {
			environment: 'disabled',
			init: function () {
				log('init');
				const sdk = this;
				return later(100).then(function () { sdk.environment = 'crazygames'; });
			},
			ad: {
				requestAd: function (kind, callbacks) {
					log('requestAd ' + kind);
					setTimeout(function () {
						callbacks.adStarted && callbacks.adStarted();
						setTimeout(function () {
							callbacks.adFinished && callbacks.adFinished();
							if (kind === 'midgame' && !settings.muteAudio) {
								settings.muteAudio = true;
								settingsListeners.forEach(function (listener) { listener({ muteAudio: true, disableChat: false }); });
							}
						}, 300);
					}, 20);
				},
				hasAdblock: function () {
					log('hasAdblock');
					return later(10, false);
				},
			},
			game: {
				settings: settings,
				loadingStart: function () { log('loadingStart'); },
				loadingStop: function () { log('loadingStop'); },
				gameplayStart: function () { log('gameplayStart'); },
				gameplayStop: function () { log('gameplayStop'); },
				happytime: function () { log('happytime'); },
				reportGameCompletedPercentage: function (percent) { log('reportGameCompletedPercentage ' + percent); },
				inviteLink: function (params) {
					log('inviteLink ' + JSON.stringify(params));
					return 'https://www.crazygames.com/game/test-game?' + Object.keys(params).map(function (key) { return key + '=' + encodeURIComponent(params[key]); }).join('&');
				},
				getInviteParam: function (name) {
					log('getInviteParam ' + name);
					return name === 'room' ? 'r42' : null;
				},
				addSettingsChangeListener: function (listener) { settingsListeners.push(listener); },
			},
			user: {
				isUserAccountAvailable: true,
				getUser: function () {
					log('getUser');
					return later(20, { username: 'user1', profilePictureUrl: 'https://example.com/user1.png' });
				},
				addAuthListener: function () {},
				getUserToken: function (options) {
					log('getUserToken ' + (options && options.signingKeyType));
					return later(20, 'fake-cg-token');
				},
				showAuthPrompt: function () {
					log('showAuthPrompt');
					return later(20, { username: 'user1' });
				},
			},
		},
	};
})();
