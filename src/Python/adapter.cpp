/*
# Copyright 2017 Stoddard, https://github.com/StoddardOXC.
 *
 * This file is a part of OpenXcom.
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

#include <unordered_map>

#include "module.h" // for the action_t
#include "adapter.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Savegame/SavedGame.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/State.h"
#include "../Engine/Logger.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ArrowButton.h"

using namespace OpenXcom;

// just so we have some lifetime to the values returned by st_translate().
std::unordered_map<std::string, std::string> translations;
void pypy_reset_translations(void) {
	translations.clear();
}

static enum SeverityLevel int2loglevel(int level) {
	switch (level) {
		default:
		case 0:
			return LOG_FATAL;
		case 1:
			return LOG_ERROR;
		case 2:
			return LOG_WARNING;
		case 3:
			return LOG_INFO;
		case 4:
			return LOG_DEBUG;
		case 5:
			return LOG_VERBOSE;
	}
}
int set_log_level(int level) {
	int old_level = Logger::reportingLevel();
	Logger::reportingLevel() = int2loglevel(level);
	return old_level;
}

void logg(int level, const char *message) {
	enum SeverityLevel l = int2loglevel(level);
	Log(l) << message;
	if (l == LOG_FATAL)
		exit(0);
}

struct _state_t : public State {
	Window *_window;
	std::string _id, _category;

	_state_t(State *parent, int32_t w, int32_t h, int32_t x, int32_t y, WindowPopup anim_type,
			 const char *ui_category, const char *background) {

		_screen = false; // wtf is this, aha, it's if it replaces the whole screen. ok.

		_window = new Window(this, w, h, x, y, anim_type);

		// here go elements' constuctors in the openxcom code.

		// "geoscape" is the ui_category
		setInterface(ui_category, true, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);

		add(_window, "window", "saveMenus");
		// here go add()-s of elements in the openxcom code

		auto bg_surface = _game->getMod()->getSurface(background);
		if (bg_surface)
			_window->setBackground(bg_surface);
		else
			Log(LOG_ERROR) << "_state_t(): failed to get background surface" << background;

		// here go elements' mutation on the openxcom code

	}
	~_state_t() {}

	// a chance to do something before elements get to it
	// for now just does nothing
	void handle(Action *action) { State::handle(action); }

	// an explicitly empty handler that does nothing.
	// used for signalling the openxcom code that the event is handled.
	// see
	void empty_handler(Action *action, State *state) { }

	// push it. do it after init, if ever.
	void push_self() { centerAllSurfaces(); _game->pushState(this); } // showAll(); } //_window->popup(); }

	// pop self
	void pop_self() { _game->popState(); }
};

/* get STR_whatever translations in utf8 */
const char* st_translate(state_t* st, const char* key) {
	auto i = translations.find(key);
	if (i == translations.end()) {
		auto trans = st->tr(key);
		auto utf8 = Language::wstrToUtf8(trans);
		translations[key] = utf8;
	}
	return translations[key].c_str();
}

// gives a bit of info to the handler, so it's not completely in the dark about what happened
static action_t convert_action(State *state, InteractiveSurface *element, Action *action, action_type_t atype) {
	action_t rv;

	rv.state = state;
	rv.element = element;
	rv.atype = atype;
	rv.is_mouse = action->isMouseAction();
	rv.kmod = SDL_GetModState();

	auto ev = action->getDetails();
	rv.key = ev->key.keysym.sym;
	rv.button = ev->button.button; // yeah, might not be defined.
	return rv;
}

// have to derive our own classes on top of supported UI elements
// to reroute events to our own hanlders.
struct _textbutton_t : public TextButton {

	_textbutton_t(int w, int32_t h, int32_t x, int32_t y, const char *text_utf8) : TextButton(w, h, x, y) {
		setText(Language::utf8ToWstr(text_utf8));
	}

	void set_click_handler(int32_t button) {
		// state that the action is handled
		onMouseClick((ActionHandler)&state_t::empty_handler, button);
		// our own handler was set in the python code.
	}
	void set_keypress_handler(const char *keyname) {
		auto optvec = Options::getOptionInfo();
		// do a table scan, we have cycles to burn here.
		// maybe later convert it to a local hash, but how to restock it after options screen?
		for (auto i = optvec.cbegin(); i != optvec.cend(); ++i) {
			if (i->type() == OPTION_KEY and i->id() == keyname) {
				onKeyboardPress((ActionHandler)(&_state_t::empty_handler), *(i->asKey()));
				return;
			}
		}
	}
	void set_anykey_handler() {
		onKeyboardPress((ActionHandler)(&_state_t::empty_handler));
	}
	virtual void mouseClick(Action *action, State *state) {
		//TextButton::mousePress(action, state);
		Log(LOG_INFO)<<"_textbutton_t::mouseClick(): handling, btn: " << ((int)action->getDetails()->button.button);
		// if we're here, the handler for the action was registered.
		// so we just set up action_t and pass it for dispatch to the python code.
		auto our_action = convert_action(state, this, action, ACTION_MOUSECLICK);
		pypy_handle_action(&our_action);
	}
	virtual void keyboardPress(Action *action, State *state) {
		//TextButton::keyboardPress(action, state);
		Log(LOG_INFO)<<"_textbutton_t::keyboardPress(): handling, keysym: "<<action->getDetails()->key.keysym.sym;
		auto our_action = convert_action(state, this, action, ACTION_KEYBOARDPRESS);
		pypy_handle_action(&our_action);
	}
};

// parent gets passed with events.
state_t *new_state(state_t *parent, int32_t w, int32_t h, int32_t x, int32_t y,
				   int32_t anim_type, const char *ui_category,  const char *background) {
	WindowPopup at_enum;
	switch (anim_type) {
		case 0:
			at_enum = POPUP_NONE;
			break;
		case 1:
			at_enum = POPUP_HORIZONTAL;
			break;
		case 2:
			at_enum = POPUP_VERTICAL;
			break;
		case 3:
		default:
			at_enum = POPUP_BOTH;
			break;
	}
	return new _state_t(parent, w, h, x, y, at_enum, ui_category, background);
}

void push_state(state_t *state) {
  state->push_self();
}

void pop_state(state_t *state) {
  state->pop_self();
}

void st_add_element(state_t *st,const char *ui_element, const char *ui_category) {
	//st->add(
}

textbutton_t *st_add_text_button(state_t *st, int32_t w, int32_t h, int32_t x, int32_t y,
								 const char *ui_element, const char *ui_category, const char *text_utf8) {
	  auto rv = new textbutton_t(w, h, x, y, text_utf8);
	  st->add(rv, ui_element, ui_category);
	  return rv;
}


void btn_set_text_utf8(textbutton_t *btn, const char *text) {
	btn->setText(Language::utf8ToWstr(text));
}

void btn_set_high_contrast(textbutton_t *btn, bool high_contrast) {
	btn->setHighContrast(high_contrast);
}

void btn_set_click_handler(textbutton_t *btn, int32_t button) {
	btn->set_click_handler(button);
}

/* keyname is a string corresponding to an OptionsInfo's _id for a keybinding */
void btn_set_keypress_handler(textbutton_t *btn, const char *key) {
	btn->set_keypress_handler(key);
}

void btn_set_anykey_handler(textbutton_t *btn) {
	btn->set_anykey_handler();
}

/* just a static text. doesn't need any action handlers or other methods, thus no object. */
void st_add_text(state_t *st, int32_t w, int32_t h, int32_t x, int32_t y,
				 const char *ui_element, const char *ui_category,
				 int32_t halign, int32_t valign, bool do_wrap, bool is_big, const char *text_utf8) {
	auto text = new Text(w, h, x, y);
	if (is_big)
		text->setBig();
	else
		text->setSmall();
	if (do_wrap)
		text->setWordWrap(do_wrap);
	text->setText(Language::utf8ToWstr(text_utf8));

	// enum TextHAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
	// enum TextVAlign { ALIGN_TOP, ALIGN_MIDDLE, ALIGN_BOTTOM };

	switch(halign) {
		default:
		case 0:
			text->setAlign(ALIGN_LEFT);
			break;
		case 1:
			text->setAlign(ALIGN_CENTER);
			break;
		case 2:
			text->setAlign(ALIGN_RIGHT);
			break;
		}
	switch(valign) {
		default:
		case 0:
			text->setVerticalAlign(ALIGN_TOP);
			break;
		case 1:
			text->setVerticalAlign(ALIGN_MIDDLE);
			break;
		case 2:
			text->setVerticalAlign(ALIGN_BOTTOM);
			break;
	}
	st->add(text, ui_element, ui_category);
}


