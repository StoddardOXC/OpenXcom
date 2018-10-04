/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Game.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cmath>
#include <string>
#include <locale>
#include <codecvt>
#include <SDL_mixer.h>
#include "State.h"
#include "Screen.h"
#include "Sound.h"
#include "Music.h"
#include "Font.h"
#include "Language.h"
#include "Logger.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Interface/Text.h"
#include "Action.h"
#include "Exception.h"
#include "Options.h"
#include "CrossPlatform.h"
#include "FileMap.h"
#include "../Menu/TestState.h"
#include <algorithm>

#include "../Python/module.h"

namespace OpenXcom
{

const double Game::VOLUME_GRADIENT = 10.0;

/**
 * Starts up SDL with all the subsystems and SDL_mixer for audio processing,
 * creates the display screen and sets up the cursor.
 * @param title Title of the game window.
 */
Game::Game(const std::string &title) : _screen(0), _cursor(0), _lang(0), _save(0), _mod(0), _quit(false), _init(false), _mouseActive(true), _timeUntilNextFrame(0)
{
	Options::reload = false;
	Options::mute = false;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "SDL initialized successfully.";

	// Initialize SDL_mixer
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
	{
		initAudio();
	}

	// trap the mouse inside the window
	SDL_WM_GrabInput(Options::captureMouse);

	// Set the window icon
	CrossPlatform::setWindowIcon(103, FileMap::getFilePath("openxcom.png"));

	// Set the window caption
	SDL_WM_SetCaption(title.c_str(), 0);

	SDL_EnableUNICODE(1);

	// Create display
	_screen = new Screen();

	// Create cursor
	_cursor = new Cursor(9, 13);

	// Create invisible hardware cursor to workaround bug with absolute positioning pointing devices
	SDL_ShowCursor(SDL_ENABLE);
	Uint8 cursor = 0;
	SDL_SetCursor(SDL_CreateCursor(&cursor, &cursor, 1,1,0,0));

	// Create fps counter
	_fpsCounter = new FpsCounter(15, 5, 0, 0);

	// Create blank language
	_lang = new Language();

	_timeOfLastFrame = 0;
}

/**
 * Deletes the display screen, cursor, states and shuts down all the SDL subsystems.
 */
Game::~Game()
{
	Sound::stop();
	Music::stop();

	for (std::list<State*>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		delete *i;
	}

	SDL_FreeCursor(SDL_GetCursor());

	delete _cursor;
	delete _lang;
	delete _save;
	delete _mod;
	delete _screen;
	delete _fpsCounter;

	Mix_CloseAudio();

	SDL_Quit();
}

/*  Exponential Moving Average.

    Used to smooth out various metrics, like framerates.

    Alpha: detemines the speed with which the returned value
		approaches the last submitted one. Defines the 'memory'
		of sorts. Typical value 0.025 for millisecond-order updates.

    Value: metric value

    Seed ceiling: number of initial values to ignore before the returned
		value can be considered meaning anything. Typical: 10.

    Reseed threshold:

    To avoid noticeable stabilization periods like after major
    metric spikes, during which no useful data is provided,
    there's the reseed_threshold parameter, which triggers
    a reseed if abs((sample-value)/value) gets larger than it
    (value being the spike value).

    So when the metric spikes for some reason its value is considerably
    larger than any previous and this triggers a reseed while dropping
    the sample so that it doesn't skew the returned value.

    This thing can be applied to graphics fps, if for some reason
    (large magnitude resize or zoom) it drops or raises abruptly.
    In this case the sample can be used as first seed value, but
    to keep the interface sane it is dropped anyway.

    Don't set it too low, or it'll keep dropping data
    and reseeding all the time.

    Default very large value disables this functionality.

    For the typical use displaying the FPS as a reciprocal of frame time
    in milliseconds, a threshold of about 10 is usually adequate.
*/

class ema_filter_t {
    float alpha;
    float value;
    float seedsum;
    unsigned seedceil;
    unsigned seedcount;
    float reseed_threshold;

public:
    ema_filter_t(float _alpha = 0.03, float _value = 0.0, unsigned _seedceil = 8, float _reseed_threshold = 3) :
        alpha(_alpha),
        value(_value),
        seedsum(0),
        seedceil(_seedceil),
        seedcount(0),
        reseed_threshold(_reseed_threshold) { }

    float update(const float sample) {
        if (fabsf((sample-value)/value) > reseed_threshold) {
            reset(sample);
            return sample;
        }

        if (seedcount < seedceil) {
            seedsum += sample;
            if (seedcount++ == seedceil) {
                value = seedsum / seedcount;
			}
        } else {
            value = alpha * sample + (1.0 - alpha) * value;
        }
        return value;
    }

    const float get(void) const { return value; }

    void reset(const float value) {
        this->value = value;
        seedcount = 0;
        seedsum = 0;
    }
};

struct EngineTimings {
	const int _w = 80, _w_dos = 160;
	const int _h = 48, _h_dos = 96;
	Text _text;
	Font *_font;
	Language *_lang;
	bool _dos;

	ema_filter_t _input, _logic, _blit, _idle, _total, _frame;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _wsconverter;

	EngineTimings() : _text(_w_dos, _h_dos, 0, 0), _font(0), _lang(0), _dos(true),
				_input(), _logic(), _blit(), _idle(), _frame(), _wsconverter() {
		_font = new Font();
		_font->loadTerminal();
		_lang = new Language();
		position(Options::baseXResolution, Options::baseYResolution);
		_text.setPalette(_font->getPalette(), 0, 2);
		_text.initText(_font, _font, _lang);
		_text.setColor(0);
		_text.setSmall();
	}
	void update(const int  inputProcessingTime, const int  logicProcessingTime, const int  blittingTime, const int idleTime) {
		_input.update(inputProcessingTime);
		_logic.update(logicProcessingTime);
		_blit.update(blittingTime);
		_idle.update(idleTime);
		_frame.update(inputProcessingTime + logicProcessingTime + blittingTime + idleTime);
	}
	void set_text_stuff(Game *game) {
		if (game->getMod() !=  NULL) {
			// so we've got a mod loaded - use its font&lang&palette.
			auto mod_font = game->getMod()->getFont("FONT_SMALL", false);
			if (mod_font != NULL ) {
				auto mod_lang = game->getLanguage();
				_text.setPalette(mod_font->getPalette());
				_text.initText(mod_font, mod_font, mod_lang);
				_text.setColor(0xf6);
				_dos = false;
			} else { // well, no mod. revert to the terminal font.
				_text.initText(_font, _font, _lang);
				_text.setPalette(_font->getPalette(), 0, 2);
				_text.setColor(0);
				_dos = true;
			}
		}
	}
	void position(int w, int h) {
		_text.setX(w- (_dos ? _w_dos : _w  - 1));
		_text.setY(0);
	}
	void blit(Surface *s, const int limit) {
		position(s->getWidth(), s->getHeight());
		_text.setPalette(s->getPalette()); // hgmm...
		_text.setVisible(true);
		_text.setHidden(false);
		_text.setAlign(ALIGN_LEFT);
		_text.setVerticalAlign(ALIGN_TOP);
		std::ostringstream actual_text;
		int ipg = round(_input.get());
		int lgg = round(_logic.get());
		int blg = round(_blit.get());
		int idg = round(_idle.get());
		int frm = round(_frame.get());
		int fps = round(1000.0/_frame.get());
		actual_text<<"input:  "<<ipg<<" ms\nlogic:  "<<lgg<<" ms\nblit:  "<<blg<<" ms\nidle:  " <<idg<<" ms\nframe:  "<<frm<<"/"<<limit<<" ms\n";
		actual_text<<"\"fps\"   "<<fps;
		_text.setText(_wsconverter.from_bytes(actual_text.str().c_str()));
		_text.draw();
		_text.blit(s);
	}
};

/**
 * The state machine takes care of passing all the events from SDL to the
 * active state, running any code within and blitting all the states and
 * cursor to the screen. This is run indefinitely until the game quits.
 */
void Game::run()
{
	enum ApplicationState { RUNNING = 0, SLOWED = 1, PAUSED = 2 } runningState = RUNNING;
	static const ApplicationState kbFocusRun[4] = { RUNNING, RUNNING, SLOWED, PAUSED };
	static const ApplicationState stateRun[4] = { SLOWED, PAUSED, PAUSED, PAUSED };
	// this will avoid processing SDL's resize event on startup, workaround for the heap allocation error it causes.
	EngineTimings engineTimings;
	bool startupEvent = Options::allowResize;
	while (!_quit)
	{
		// Clean up states
		while (!_deleted.empty())
		{
			delete _deleted.back();
			_deleted.pop_back();
		}

		// Initialize active state
		if (!_init)
		{
			_init = true;
			_states.back()->init();

			// Unpress buttons
			_states.back()->resetAll();

			// Refresh mouse position
			SDL_Event ev;
			int x, y;
			SDL_GetMouseState(&x, &y);
			ev.type = SDL_MOUSEMOTION;
			ev.motion.x = x;
			ev.motion.y = y;
			Action action = Action(&ev, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
			_states.back()->handle(&action);
		}

		auto frameStartedAt = SDL_GetTicks();
		// Process events
		while (SDL_PollEvent(&_event))
		{
			if (CrossPlatform::isQuitShortcut(_event))
				_event.type = SDL_QUIT;
			switch (_event.type)
			{
				case SDL_QUIT:
					quit();
					break;
				case SDL_ACTIVEEVENT:
					switch (reinterpret_cast<SDL_ActiveEvent*>(&_event)->state)
					{
						case SDL_APPACTIVE:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain ? RUNNING : stateRun[Options::pauseMode];
							break;
						case SDL_APPMOUSEFOCUS:
							// We consciously ignore it.
							break;
						case SDL_APPINPUTFOCUS:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain ? RUNNING : kbFocusRun[Options::pauseMode];
							break;
					}
					break;
				case SDL_VIDEORESIZE:
					if (Options::allowResize)
					{
						if (!startupEvent)
						{
							Options::newDisplayWidth = Options::displayWidth = std::max(Screen::ORIGINAL_WIDTH, _event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(Screen::ORIGINAL_HEIGHT, _event.resize.h);
							int dX = 0, dY = 0;
							Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, false);
							Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, false);
							for (std::list<State*>::iterator i = _states.begin(); i != _states.end(); ++i)
							{
								(*i)->resize(dX, dY);
							}
							_screen->resetDisplay();
						}
						else
						{
							startupEvent = false;
						}
					}
					break;
				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					// Skip mouse events if they're disabled
					if (!_mouseActive) continue;
					// re-gain focus on mouse-over or keypress.
					runningState = RUNNING;
					// Go on, feed the event to others
				default:
					Action action = Action(&_event, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
					_screen->handle(&action);
					_cursor->handle(&action);
					_fpsCounter->handle(&action);
					if (action.getDetails()->type == SDL_KEYDOWN)
					{
						// "ctrl-g" grab input
						if (action.getDetails()->key.keysym.sym == SDLK_g && (SDL_GetModState() & KMOD_CTRL) != 0)
						{
							Options::captureMouse = (SDL_GrabMode)(!Options::captureMouse);
							SDL_WM_GrabInput(Options::captureMouse);
						}
						else if (Options::debug)
						{
							if (action.getDetails()->key.keysym.sym == SDLK_t && (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								pushState(new TestState);
							}
							// "ctrl-u" debug UI
							else if (action.getDetails()->key.keysym.sym == SDLK_u && (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								Options::debugUi = !Options::debugUi;
								_states.back()->redrawText();
							}
						}
					}
					_states.back()->handle(&action);
					break;
			}
		}
		Uint32 frameNominalDuration = 1; // 1e3 FPS should be enough for anybody
		if (Options::FPS > 0 )
		{
			auto fpsLimit = SDL_GetAppState() & SDL_APPINPUTFOCUS ? Options::FPS : Options::FPSInactive;
			frameNominalDuration = 1000 / fpsLimit;
		}
		auto inputProcessedAt = SDL_GetTicks();
		auto inputProcessingTime = inputProcessedAt - frameStartedAt;
		auto logicStartedAt = inputProcessedAt;
		// process logic
		if (runningState != PAUSED)
		{
			_states.back()->think();
		}
		auto blitStartedAt = SDL_GetTicks();
		auto logicProcessingTime = blitStartedAt - logicStartedAt;

		// do the blit
		auto screenSurface = _screen->getSurface();
		screenSurface->clear();

		// find the topmost full-screen state
		auto i = _states.end();
		for (--i; i != _states.begin() && !(*i)->isScreen(); --i);

		// blit it and every other state on top of it
		for (; i != _states.end(); (*(i++))->blit());

		if (Options::fpsCounter)
		{
			engineTimings.set_text_stuff(this);
			engineTimings.blit(screenSurface, frameNominalDuration);
		}
		_cursor->blit(screenSurface);
		_screen->flip();

		auto blitDoneAt = SDL_GetTicks();
		auto blittingTime = blitDoneAt - blitStartedAt;

		auto frameTimeSpentSoFar = blitDoneAt - frameStartedAt;
		int32_t idleTime = frameNominalDuration - frameTimeSpentSoFar;

		// update the stats
		engineTimings.update(inputProcessingTime, logicProcessingTime, blittingTime, idleTime);

		// sleep until next frame
		if (idleTime > 1)  // 1 is to have some leg room if we're that close to the limit
		{
			SDL_Delay(idleTime);
		}
	}
	Options::save();
}

/**
 * Stops the state machine and the game is shut down.
 */
void Game::quit()
{
	// Always save ironman
	if (_save != 0 && _save->isIronman() && !_save->getName().empty())
	{
		std::string filename = CrossPlatform::sanitizeFilename(Language::wstrToFs(_save->getName())) + ".sav";
		_save->save(filename, _mod);
	}
	_quit = true;
}

/**
 * Changes the audio volume of the music and
 * sound effect channels.
 * @param sound Sound volume, from 0 to MIX_MAX_VOLUME.
 * @param music Music volume, from 0 to MIX_MAX_VOLUME.
 * @param ui UI volume, from 0 to MIX_MAX_VOLUME.
 */
void Game::setVolume(int sound, int music, int ui)
{
	if (!Options::mute)
	{
		if (sound >= 0)
		{
			sound = volumeExponent(sound) * (double)SDL_MIX_MAXVOLUME;
			Mix_Volume(-1, sound);
			if (_save && _save->getSavedBattle())
			{
				Mix_Volume(3, sound * _save->getSavedBattle()->getAmbientVolume());
			}
			else
			{
				// channel 3: reserved for ambient sound effect.
				Mix_Volume(3, sound / 2);
			}
		}
		if (music >= 0)
		{
			music = volumeExponent(music) * (double)SDL_MIX_MAXVOLUME;
			Mix_VolumeMusic(music);
		}
		if (ui >= 0)
		{
			ui = volumeExponent(ui) * (double)SDL_MIX_MAXVOLUME;
			Mix_Volume(1, ui);
			Mix_Volume(2, ui);
		}
	}
}

double Game::volumeExponent(int volume)
{
	return (exp(log(Game::VOLUME_GRADIENT + 1.0) * volume / (double)SDL_MIX_MAXVOLUME) -1.0 ) / Game::VOLUME_GRADIENT;
}

/**
 * Returns the display screen used by the game.
 * @return Pointer to the screen.
 */
Screen *Game::getScreen() const
{
	return _screen;
}

/**
 * Returns the mouse cursor used by the game.
 * @return Pointer to the cursor.
 */
Cursor *Game::getCursor() const
{
	return _cursor;
}

/**
 * Returns the FpsCounter used by the game.
 * @return Pointer to the FpsCounter.
 */
FpsCounter *Game::getFpsCounter() const
{
	return _fpsCounter;
}

/**
 * Pops all the states currently in stack and pushes in the new state.
 * A shortcut for cleaning up all the old states when they're not necessary
 * like in one-way transitions.
 * @param state Pointer to the new state.
 */
void Game::setState(State *state)
{
	while (!_states.empty())
	{
		popState();
	}
	pushState(state);
	_init = false;
}

/**
 * Pushes a new state into the top of the stack and initializes it.
 * The new state will be used once the next game cycle starts.
 * @param state Pointer to the new state.
 */
void Game::pushState(State *state)
{
	_states.push_back(state);
	_init = false;
}

/**
 * Pops the last state from the top of the stack. Since states
 * can't actually be deleted mid-cycle, it's moved into a separate queue
 * which is cleared at the start of every cycle, so the transition
 * is seamless.
 */
void Game::popState()
{
	_deleted.push_back(_states.back());
	_states.pop_back();
	_init = false;
}

/**
 * Returns the language currently in use by the game.
 * @return Pointer to the language.
 */
Language *Game::getLanguage() const
{
	return _lang;
}

/**
 * Changes the language currently in use by the game.
 * @param filename Filename of the language file.
 */
void Game::loadLanguage(const std::string &filename)
{
	const std::string dirLanguage = "/Language/";
	const std::string dirLanguageAndroid = "/Language/Android/";
	const std::string dirLanguageOXCE = "/Language/OXCE/";
	const std::string dirLanguageTechnical = "/Language/Technical/";

	// Step 1: openxcom "common" strings
	loadLanguageCommon(filename, dirLanguage, false);
	loadLanguageCommon(filename, dirLanguageAndroid, true);
	loadLanguageCommon(filename, dirLanguageOXCE, true);
	loadLanguageCommon(filename, dirLanguageTechnical, true);

	// Step 2: mod strings (note: xcom1 and xcom2 are also "standard" mods)
	std::vector<const ModInfo*> activeMods = Options::getActiveMods();
	for (std::vector<const ModInfo*>::const_iterator i = activeMods.begin(); i != activeMods.end(); ++i)
	{
		// if a master mod (e.g. piratez) has a master (e.g. xcom1), load it too (even though technically it is not enabled)
		if ((*i)->isMaster() && (*i)->getMaster().length() > 1)
		{
			const ModInfo *masterModInfo = &Options::getModInfos().at((*i)->getMaster());
			loadLanguageMods(masterModInfo, filename, dirLanguage);
			loadLanguageMods(masterModInfo, filename, dirLanguageAndroid);
			loadLanguageMods(masterModInfo, filename, dirLanguageOXCE);
			loadLanguageMods(masterModInfo, filename, dirLanguageTechnical);
		}
		// now load the mod itself
		loadLanguageMods((*i), filename, dirLanguage);
		loadLanguageMods((*i), filename, dirLanguageAndroid);
		loadLanguageMods((*i), filename, dirLanguageOXCE);
		loadLanguageMods((*i), filename, dirLanguageTechnical);
	}

	// Step 3: mod extra-strings (from all mods at once)
	const std::map<std::string, ExtraStrings*> &extraStrings = _mod->getExtraStrings();
	std::map<std::string, ExtraStrings*>::const_iterator it = extraStrings.find(filename);
	if (it != extraStrings.end())
	{
		_lang->load(it->second);
	}
	pypy_lang_changed();
}

void Game::loadLanguageCommon(const std::string &filename, const std::string &directory, bool checkIfExists)
{
	std::ostringstream ss;
	ss << directory << filename << ".yml";
	std::string path = CrossPlatform::searchDataFile("common" + ss.str());
	try
	{
		if (checkIfExists && !CrossPlatform::fileExists(path))
		{
			return;
		}
		_lang->load(path);
	}
	catch (YAML::Exception &e)
	{
		throw Exception(path + ": " + std::string(e.what()));
	}
}

void Game::loadLanguageMods(const ModInfo *modInfo, const std::string &filename, const std::string &directory)
{
	std::ostringstream ss;
	ss << directory << filename << ".yml";
	std::string file = modInfo->getPath() + ss.str();
	if (CrossPlatform::fileExists(file))
	{
		_lang->load(file);
	}
}

/**
 * Returns the saved game currently in use by the game.
 * @return Pointer to the saved game.
 */
SavedGame *Game::getSavedGame() const
{
	return _save;
}

/**
 * Sets a new saved game for the game to use.
 * @param save Pointer to the saved game.
 */
void Game::setSavedGame(SavedGame *save)
{
	delete _save;
	_save = save;
}

/**
 * Returns the mod currently in use by the game.
 * @return Pointer to the mod.
 */
Mod *Game::getMod() const
{
	return _mod;
}

/**
 * Loads the mods specified in the game options.
 */
void Game::loadMods()
{
	Mod::resetGlobalStatics();
	delete _mod;
	_mod = new Mod();
	_mod->loadAll(FileMap::getRulesets());
}

/**
 * Sets whether the mouse is activated.
 * If it is, mouse events are processed, otherwise
 * they are ignored and the cursor is hidden.
 * @param active Is mouse activated?
 */
void Game::setMouseActive(bool active)
{
	_mouseActive = active;
	_cursor->setVisible(active);
}

/**
 * Returns whether current state is *state
 * @param state The state to test against the stack state
 * @return Is state the current state?
 */
bool Game::isState(State *state) const
{
	return !_states.empty() && _states.back() == state;
}

/**
 * Checks if the game is currently quitting.
 * @return whether the game is shutting down or not.
 */
bool Game::isQuitting() const
{
	return _quit;
}

/**
 * Loads the most appropriate language
 * given current system and game options.
 */
void Game::defaultLanguage()
{
	const std::string defaultLang = "en-US";
	std::string currentLang = defaultLang;

	delete _lang;
	_lang = new Language();

	std::ostringstream ss;
	ss << "common/Language/" << defaultLang << ".yml";
	std::string defaultPath = CrossPlatform::searchDataFile(ss.str());
	std::string path = defaultPath;

	// No language set, detect based on system
	if (Options::language.empty())
	{
		std::string locale = CrossPlatform::getLocale();
		std::string lang = locale.substr(0, locale.find_first_of('-'));
		// Try to load full locale
		Language::replace(path, defaultLang, locale);
		if (CrossPlatform::fileExists(path))
		{
			currentLang = locale;
		}
		else
		{
			// Try to load language locale
			Language::replace(path, locale, lang);
			if (CrossPlatform::fileExists(path))
			{
				currentLang = lang;
			}
			// Give up, use default
			else
			{
				currentLang = defaultLang;
			}
		}
	}
	else
	{
		// Use options language
		Language::replace(path, defaultLang, Options::language);
		if (CrossPlatform::fileExists(path))
		{
			currentLang = Options::language;
		}
		// Language not found, use default
		else
		{
			currentLang = defaultLang;
		}
	}

	loadLanguage(defaultLang);
	if (currentLang != defaultLang)
	{
		loadLanguage(currentLang);
	}
	Options::language = currentLang;
}

/**
 * Initializes the audio subsystem.
 */
void Game::initAudio()
{
	Uint16 format;
	if (Options::audioBitDepth == 8)
		format = AUDIO_S8;
	else
		format = AUDIO_S16SYS;
	if (Options::audioSampleRate >= 44100)
		Options::audioChunkSize = std::max(2048, Options::audioChunkSize);
	else if (Options::audioSampleRate >= 22050)
		Options::audioChunkSize = std::max(1024, Options::audioChunkSize);
	else if (Options::audioSampleRate >= 11025)
		Options::audioChunkSize = std::max(512, Options::audioChunkSize);
	if (Mix_OpenAudio(Options::audioSampleRate, format, 2, Options::audioChunkSize) != 0)
	{
		Log(LOG_ERROR) << Mix_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
	{
		Mix_AllocateChannels(16);
		// Set up UI channels
		Mix_ReserveChannels(4);
		Mix_GroupChannels(1, 2, 0);
		Log(LOG_INFO) << "SDL_mixer initialized successfully.";
		setVolume(Options::soundVolume, Options::musicVolume, Options::uiVolume);
	}
}

}
