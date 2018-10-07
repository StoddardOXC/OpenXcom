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
#include <unordered_set>
#include <cmath>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#include "SDL_mixer.h"

#include "module.h"
#include "adapter.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Sound.h"
#include "../Engine/Screen.h"
#include "../Engine/FileMap.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/State.h"
#include "../Engine/Logger.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Menu/ErrorMessageState.h"

#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/Armor.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/Production.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"

using namespace OpenXcom;

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _wsconverter;
static std::wstring utf8_to_wstr(const char *str) { return  _wsconverter.from_bytes(str); }
static std::wstring utf8_to_wstr(const std::string &str) { return  _wsconverter.from_bytes(str); }

//{ logging
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
//}
//{ The Game object is in fact static in the lifetime of main(). Why does anyone pretend otherwise?
//  This is not a multiboard chess game, period.
Game *GAME = NULL;
void set_game_ptr(void *g) {
	GAME = reinterpret_cast<Game *>(g);
}
//}
//{ The most important error message
void *state_not_found(const char *name) {
	std::string nstr(name);
	std::string msg("Attempt to spawn an undefined state '" + nstr + "'\nPlease check enabled modules.");
	std::string bg = "BACK01.SCR";
	Uint8 color = GAME->getMod()->getInterface("errorMessages")->getElement("geoscapeColor")->color;
	if ((GAME->getSavedGame() != 0) && (GAME->getSavedGame()->getSavedBattle() != 0)) {
		color = GAME->getMod()->getInterface("errorMessages")->getElement("battlescapeColor")->color;
		bg = "TAC00.SCR";
	}
	return new ErrorMessageState(utf8_to_wstr(msg), GAME->getScreen()->getPalette(), color, bg ,-1);
}
//}
//{ ui hook
/* this state is an ui container.
 * exists to grab input from and return a single surface to the engine
 * incidentally helds some stuff like strings for passing into python code
*/
struct _state_t : public State {
	void *_ffihandle;
	Surface *_surface;
	int32_t _w, _h, _x, _y;
	double _xscale, _yscale;
	int32_t _left, _top;
	std::string _id, _category;
	std::vector<const std::string *> _interned_strings;
	std::vector<Surface *> _interned_surfaces;
	std::vector<Text *> _interned_texts;

	// input state
	st_input_t _st;

	_state_t(void *ffih, int32_t w, int32_t h, int32_t x, int32_t y, const std::string &id, const std::string &category, bool alterpal) {
		_ffihandle = ffih;
		_screen = false; // it's if it replaces the whole screen, so no.
		_w = w; _h = h; _x = x; _y = y;
		_xscale = 1.0; _yscale = 1.0;
		_left = 0; _top = 0;
		_surface = new Surface(w, h, x, y); // our blit target
		setInterface(category, alterpal, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);
		add(_surface, id, category); // this calls setPalette on it... hmm. ah, to hell with it.

		_st.buttons = 0;
		_st.mx = 0;
		_st.my = 0;
		_st.keysym = -1;
		_st.keymod = 0;
		_st.unicode = -1;
	}
	/// Initializes the state. All done in the constructor. Just activates itself here.
	virtual void init() { /* _game->pushState(this); */}

	void set_mb_state(int button, bool pressed) {
		if (pressed) {
			_st.buttons |= 1<<(button - 1);
		} else {
			_st.buttons &= ~(1<<(button -1));
		}
	}
	// converts coords, also checks if it's in our box.
	bool set_mouse_posn(int sx, int sy) {
		// well duh. st, sy,  _left,_top are in system pixels, while _x is in logical pixels. as are _w and _h
		// to reduce confusion,
		// first decide if we're letterboxxedd?
		int sysleft = round(_x  *_xscale) + _left;
		int systop = round(_y * _yscale) + _top;
		int sysright = sysleft + round(_w * _xscale);
		int sysbottom = systop + round(_h * _yscale);
		if ((sx < sysleft) or (sy < systop) or (sx >= sysright) or (sy >= sysbottom)) {
			// outside of our box
			//Log(LOG_INFO)<<"mouseposn: sx="<<sx<<" sy="<<sy<<" left="<<sysleft<<" top="<<systop<<" right="<<sysright<<" bottom="<<sysbottom;
			return false;
		}
		// calculate mouse coords inside our window/surface/whatever
		_st.mx = round((sx - sysleft)/_xscale);
		_st.my = round((sy - systop)/_yscale);
		//Log(LOG_INFO)<<"mouseposn: got "<<_st.mx<<":"<<_st.my;
		return true;
	}
	// pass input events up to pypy code
	// Action gets here before the souping up in InteractiveSurface::handle
	// so we must do the same here. Besides the SDL_Event, this provides
	// scale and offset of the draw area so that we can transform the mouse coords
	// and filter'em based on coords.
	//
	// to get rid of input drops due to low frame rate
	// we can run the logic for each event without rendering
	// then run it again for the last event with rendering. hmm. let's see.
	virtual void handle(Action *action) {
		bool pass_on = false;
		_xscale = action->getXScale();
		_yscale = action->getYScale();
		_left = action->getLeftBlackBand();
		_top = action->getTopBlackBand();
		SDL_Event *ev = action->getDetails();
		switch (ev->type) {
			case SDL_MOUSEBUTTONUP:
				if(set_mouse_posn(ev->button.x, ev->button.y)) {
					set_mb_state(ev->button.button, false);
					//Log(LOG_INFO) << "SDL_MOUSEBUTTONUP: " << ((int)(ev->button.button));
					pass_on = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(set_mouse_posn(ev->button.x, ev->button.y)) {
					set_mb_state(ev->button.button, true);
					//Log(LOG_INFO) << "SDL_MOUSEBUTTONDOWN: " << ((int)(ev->button.button));
					pass_on = true;
				}
				break;
			case SDL_MOUSEMOTION:
				if(set_mouse_posn(ev->motion.x, ev->motion.y)) {
					pass_on = true;
				}
				break;
			case SDL_KEYDOWN:
				_st.keysym = ev->key.keysym.sym;
				_st.keymod = ev->key.keysym.mod;
				//Log(LOG_INFO) << "SDL_KEYDOWN keysym.sym=" << ((int)(ev->key.keysym.sym));
				pass_on = true;
				break;
			case SDL_KEYUP:
				break;
			default:
				break;
		}
		if (pass_on) {
			pypy_call_method(_ffihandle, "_inner_handle");
			//pypy_state_input(this);
		}
		// reset kbd data
		_st.keysym = -1;
		_st.unicode = -1;
		// keep _keymod
		//State::handle(action); why?
	}

	/// Runs state functionality every cycle.
	virtual void think() {
		pypy_call_method(_ffihandle, "_inner_think");
	}

	/// Blits the state to the screen.
	virtual void blit() {
		pypy_call_method(_ffihandle, "_inner_blit");
		//pypy_state_blit(this); // draw to the underlying surface
		State::blit(); // blit to the actual screen
	}

	virtual void resize(int &dX, int &dY) { Log(LOG_ERROR)<< "state_t::resize(): not implemented. " << dX << ":" <<dY; }

	// get game
	static Game *get_game() { return GAME; }

	// copy and keep a string for the lifetime of the state
	const char *intern_string(const std::string& s) {
		auto *is = new std::string(s);
		_interned_strings.push_back(is);
		return is->c_str();
	}
	// same for the disgusting wstrings
	const char *intern_string(const std::wstring &ws) {
		return intern_string(Language::wstrToUtf8(ws));
	}
	// just keep a string for the lifetime of the state
	const char *intern_string(const std::string *is) {
		_interned_strings.push_back(is);
		return is->c_str();
	}
	void intern_surface(Surface *s) {
		_interned_surfaces.push_back(s);
	}
	void intern_text(Text *t) {
		_interned_texts.push_back(t);
	}
	~_state_t() {
		// _surface is deleted in State destructor
		// interned surfaces are supposed to be mods' surfaces so no deletion here
		for (auto is: _interned_strings) { delete is; }
		for (auto it: _interned_texts) { delete it; }
	}
};

state_t *new_state(void *ffih, int32_t w, int32_t h, int32_t x, int32_t y, const char *ui_id, const char *ui_category, int32_t alterpal) {
	return new state_t(ffih, w, h, x, y, ui_id, ui_category, alterpal);
}
void push_state(state_t *state) { GAME->pushState(state); }
void pop_state(state_t *state) { GAME->popState(state); } /* delete is done in the main loop */
void st_clear(state_t *state) { state->_surface->clear(0); }
void st_fill(state_t *state, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) { state->_surface->drawRect(x, y, w, h, color); }
void st_blit(state_t *state, int32_t dst_x, int32_t dst_y, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h, uintptr_t upsrc) {
	Surface *src = reinterpret_cast<Surface *>(upsrc);
	src->setX(dst_x);
	src->setY(dst_y);
	src->getCrop()->x = src_x;
	src->getCrop()->y = src_y;
	src->getCrop()->w = src_w;
	src->getCrop()->h = src_h;
	src->blit(state->_surface);
}
uintptr_t st_clone_surface(state_t *state, uintptr_t surf) {
	Surface *src = reinterpret_cast<Surface *>(surf);
	Surface *clone = new Surface(*src);
	state->intern_surface(clone);
	return reinterpret_cast<uintptr_t>(clone);
}
void st_invert_surface(state_t *state, uintptr_t surf, int32_t invert_mid) {
	Surface *src = reinterpret_cast<Surface *>(surf);
	src->invert(invert_mid);
}
uintptr_t st_intern_surface(state_t *state, const char *name) {
	auto src = state->get_game()->getMod()->getSurface(name);
	if (src == NULL) {
		Log(LOG_ERROR) << "Surface '" << name << "' not found";
		return 0;
	}
	state->intern_surface(src);
	return reinterpret_cast<uintptr_t>(src);
}
uintptr_t st_intern_sub(state_t *state, const char *name, int32_t idx) {
	auto srcset = state->get_game()->getMod()->getSurfaceSet(name, false);
	if (srcset == NULL) {
		Log(LOG_ERROR) << "SurfaceSet '" << name << "' not found";
		return 0;
	}
	auto src = srcset->getFrame(idx);
	if (src == NULL) {
		Log(LOG_ERROR) << "Surface '" << name << "[" << idx << "]' not found";
		return 0;
	}
	state->intern_surface(src);
	return reinterpret_cast<uintptr_t>(src);
}
uintptr_t st_intern_text(state_t *state, int32_t w, int32_t h, const char *text, int32_t halign, int32_t valign, int32_t wrap,
		int32_t primary, int32_t secondary, int32_t is_small) {

	auto _text = new Text(w, h, 0, 0);
	_text->initText(state->get_game()->getMod()->getFont("FONT_BIG"),
					state->get_game()->getMod()->getFont("FONT_SMALL"),
					state->get_game()->getLanguage());
	if (is_small) { _text->setSmall(); } else { _text->setBig(); }
	_text->setPalette(state->getPalette());
	_text->setColor(primary);
	_text->setSecondaryColor(secondary);
	switch (halign) {
		case 0:
			_text->setAlign(ALIGN_LEFT);
			break;
		case 1:
			_text->setAlign(ALIGN_CENTER);
			break;
		case 2:
			_text->setAlign(ALIGN_RIGHT);
			break;
	}
	switch (valign) {
		case 0:
			_text->setVerticalAlign(ALIGN_TOP);
			break;
		case 1:
			_text->setVerticalAlign(ALIGN_MIDDLE);
			break;
		case 2:
			_text->setVerticalAlign(ALIGN_BOTTOM);
			break;
	}
	_text->setText(utf8_to_wstr(text));
	_text->draw();
	state->intern_text(_text);
	return reinterpret_cast<uintptr_t>(_text);
}
void st_play_ui_sound(state_t *state, const char *name) {
	auto s = std::string(name);
	if (s == "BUTTON_PRESS") { if (TextButton::soundPress) TextButton::soundPress->play(Mix_GroupAvailable(0)); }
	else if (s == "WINDOW_POPUP_0") { if (Window::soundPopup[0]) Window::soundPopup[0]->play(Mix_GroupAvailable(0)); }
	else if (s == "WINDOW_POPUP_1") { if (Window::soundPopup[1]) Window::soundPopup[1]->play(Mix_GroupAvailable(0)); }
	else if (s == "WINDOW_POPUP_2") { if (Window::soundPopup[2]) Window::soundPopup[2]->play(Mix_GroupAvailable(0)); }
	else { Log(LOG_ERROR) << "unsuppored ui sound '" << s << "'"; }
}
st_input_t *st_get_input_state(state_t *state) { return &state->_st; }
//}
//{ translations
// just so we have some lifetime to the values returned by st_translate().
std::unordered_map<std::string, std::string> translations;
void drop_translations(void) {
	translations.clear();
}
/* get STR_whatever translations in utf8 */
const char *st_translate(state_t *state, const char *key) {
	auto i = translations.find(key);
	if (i == translations.end()) {
		auto trans = state->tr(key);
		auto utf8 = Language::wstrToUtf8(trans);
		translations[key] = utf8;
	}
	return translations[key].c_str();
}
const char *st_translate(state_t *st, const std::string& key) {
	return st_translate(st, key.c_str());
}
//}
/* ohhh now we get to hold per-mod state like what an armor is. */
std::unordered_set<std::string> static_craft_weapon_types;
std::unordered_set<std::string> static_armor_types;
std::unordered_set<std::string> static_item_categories;
std::unordered_set<std::string> static_precursor_items;
std::string static_interface_rules;

const char *get_interface_rules() { return static_interface_rules.c_str(); }

void prepare_static_mod_data() {
	Game *game = GAME;

	static_craft_weapon_types.clear();
	auto& cw = game->getMod()->getCraftWeaponsList();
	for (auto i = cw.begin(); i != cw.end(); ++i) {
		RuleCraftWeapon *rule = game->getMod()->getCraftWeapon(*i);
		static_craft_weapon_types.insert(rule->getLauncherItem());
		static_craft_weapon_types.insert(rule->getClipItem());
	}
	static_armor_types.clear();
	auto& ar = game->getMod()->getArmorsList();
	for (auto i = ar.begin(); i != ar.end(); ++i) {
		Armor *rule = game->getMod()->getArmor(*i);
		static_armor_types.insert(rule->getStoreItem());
	}
	static_item_categories.clear();
	auto& cats = game->getMod()->getItemCategoriesList();
	for (auto i = cats.cbegin(); i != cats.cend(); ++i) {
		static_item_categories.insert(*i);
	}
	static_precursor_items.clear();
	auto& manu_projects = game->getMod()->getManufactureList();
	for (auto mpi = manu_projects.cbegin(); mpi != manu_projects.cend(); ++mpi ) {
		auto rule = game->getMod()->getManufacture(*mpi);
		auto& reqs = rule->getRequiredItems();
		for (auto req = reqs.begin(); req != reqs.end(); ++req) {
			static_precursor_items.insert((*req).first->getType());
		}
	}
	game->getMod()->getInterfacesYaml(static_interface_rules);
}
// stuff that changes with research progression
// dat basically all projects with required_items methinks.
std::unordered_set<std::string> dynamic_discovered_research;
std::unordered_map<std::string, int32_t> dynamic_researchable_items; // see researchablessness_type_t in .h
// Should be called every time a research project completes. I think that would be enough.
void prepare_researchable_items_map(uintptr_t _game) {
	Game *game = reinterpret_cast<Game *>(_game);
	dynamic_researchable_items.clear();
	dynamic_discovered_research.clear();

	auto& discoveredResearch = game->getSavedGame()->getDiscoveredResearch();
	for (auto it = discoveredResearch.begin(); it != discoveredResearch.end(); ++it) {
			dynamic_discovered_research.insert((*it)->getName());
	}
	auto& rl = game->getMod()->getResearchList();
	for (auto rit = rl.begin(); rit != rl.end(); ++rit) {
		auto rule = game->getMod()->getResearch(*rit);
		if (rule->needItem()) {
			/* I forgot what lookup means... well, later. */
			int32_t researchablessness;
			if (dynamic_discovered_research.find(rule->getName()) != dynamic_discovered_research.end()) {
				researchablessness = RLNT_DONE;
			} else if (false) {
				// all dependencies satisfied ?
				// ow. this is complicated. should maybe do this outside of here.
				researchablessness = RLNT_RIGHT_NOW;
			} else {
				// something missing...
				researchablessness = RLNT_MISSES_DEPENDENCY;
			}
			dynamic_researchable_items.insert(std::pair<std::string, int32_t>(rule->getName(), researchablessness));
		}
	}
}
#if 0
// changes on research and facility completion. so better call it on every inventory fetch I think.
// it's actually a set... maybe. TODO.
std::unordered_set<std::pair<std::string, int32_t>> *get_purchasable_items_for_base(Game *game, Base *base) {
	auto rv = new std::unordered_set<std::pair<std::string, int32_t>>; // item_type, meta_item_type
	rv->clear();
	return rv;
}
#endif

bool get_precursor_state(const std::string& item_type) {
	return static_precursor_items.find(item_type) != static_precursor_items.end();
}
int32_t get_researchablelessness_state(const std::string& item_type) {
	auto it = dynamic_researchable_items.find(item_type);
	if (it == dynamic_researchable_items.end()) {
		Log(LOG_WARNING) << "no researchablelessness state found for '" << item_type << "'";
		return RLNT_DONE;
	}
	return (*it).second;
}
int32_t guess_item_meta_type(const std::string& item_type, RuleItem *rule) {
	if (rule->getBattleType() == BT_NONE) {
		if (static_craft_weapon_types.find(item_type) != static_craft_weapon_types.end()) {
			return META_CRAFT_EQUPMENT;
		}
		if (static_armor_types.find(item_type) != static_armor_types.end()) {
			return META_ARMOR;
		}
		return META_GENERIC_ITEM;
	}
	return META_EQUIPMENT;
}
//{ base inventory
int32_t get_base_facilities(state_t *st, int32_t base_idx, base_t *base, facility_t *fac_vec, size_t fac_cap) {
	base->name = NULL;
	auto _game = st->get_game();
	if (base_idx < 0 or _game->getSavedGame() == NULL) {
		return -1;
	}

	auto bases_vec = _game->getSavedGame()->getBases();
	if (bases_vec->size() <= base_idx) {
		return -1;
	}

	auto _base = bases_vec->at(base_idx);
	memset(fac_vec, 0, sizeof(facility_t) * fac_cap);
	auto _facilities = _base->getFacilities();

	size_t fac_idx = 0;
	for (auto it = _facilities->begin(); it != _facilities->end() and fac_idx != fac_cap; ++it, ++fac_idx) {
		auto rule = (*it)->getRules();
		fac_vec[fac_idx].handle = reinterpret_cast<uintptr_t>(*it);
		fac_vec[fac_idx].online = not (*it)->getDisabled() and (*it)->getBuildTime() == 0;
		fac_vec[fac_idx].id = st->intern_string(rule->getType());
		fac_vec[fac_idx].ware_capacity = rule->getStorage();
		fac_vec[fac_idx].crew_capacity = rule->getPersonnel();
		fac_vec[fac_idx].craft_capacity = rule->getCrafts();
		fac_vec[fac_idx].jail_type = rule->getPrisonType();
		fac_vec[fac_idx].jail_capacity = rule->getAliens();
		fac_vec[fac_idx].maintenance = rule->getMonthlyCost();
	}

	base->idx = base_idx;
	base->handle = reinterpret_cast<uintptr_t>(_base);

	base->name = st->intern_string(Language::wstrToUtf8(_base->getName()));
	base->facility_count = _facilities->size();
	base->inventory_count =
		(_base->getEngineers() > 0 ? 2 : 0) +   // 2 because busy and idle are shown separately
		(_base->getScientists() > 0 ? 2 : 0) +
		_base->getSoldiers()->size() +
		_base->getCrafts()->size() +
		_base->getVehicles()->size() +  // this is wrong, overstates the number b/c doesn't take type into account.
		_base->getTransfers()->size() +
		_base->getStorageItems()->getContents()->size();

	for (auto it = _base->getCrafts()->begin(); it != _base->getCrafts()->end(); ++it) {
		base->inventory_count += (*it)->getItems()->getContents()->size();
		base->inventory_count += (*it)->getVehicles()->size(); // same as the above
		base->inventory_count += (*it)->getWeapons()->size() * 2; // even more overstatement
	}

	// TODO: get items that are being researched and add a slot per one.
	// or we miss something

	return _facilities->size() < fac_cap ? _facilities->size() : fac_cap;
}
int32_t item_inventory(state_t *st, const std::string& item_type, int32_t amount, item_t *item_vec, int32_t item_idx, int32_t box_idx, int32_t meta, bool busy) {
	auto item_rule = st->get_game()->getMod()->getItem(item_type);
	if (item_rule == NULL) {
		Log(LOG_WARNING) << "item_inventory(): No item ruleset for type '" << item_type << "'";
		return item_idx;
	}
	item_vec[item_idx].box_idx = box_idx;
	if (meta == META_GENERIC_ITEM) {
		meta = guess_item_meta_type(item_type, item_rule);
	}
	item_vec[item_idx].meta = meta;
	item_vec[item_idx].amount = amount;
	item_vec[item_idx].id = st->intern_string(item_type);
	item_vec[item_idx].buy_price  = item_rule->getBuyCost();
	item_vec[item_idx].sell_price  = item_rule->getSellCost();
	//item_vec[item_idx].rent_price  = item_rule->getRentCost();
	item_vec[item_idx].ware_size  = item_rule->getSize();
	item_vec[item_idx].cats = 0;
	item_vec[item_idx].pedia_unlocked  = 0; /// TODO
	item_vec[item_idx].researchable  = get_researchablelessness_state(item_type);
	item_vec[item_idx].precursor  = get_precursor_state(item_type);
	item_vec[item_idx].in_processing = busy;
	item_vec[item_idx].autosell = st->get_game()->getSavedGame()->getAutosell(item_rule);
	return item_idx + 1;
}
int32_t soldier_inventory(state_t *st, Soldier *soul, item_t *item_vec, int32_t item_idx, int32_t box_idx, bool busy) {
	auto rules = soul->getRules();
	item_vec[item_idx].handle = reinterpret_cast<uintptr_t>(soul);
	item_vec[item_idx].box_idx = box_idx;
	item_vec[item_idx].amount = 1;
	item_vec[item_idx].id = st->intern_string("STR_SOLDIER");
	item_vec[item_idx].name= st->intern_string(soul->getName(true));
	item_vec[item_idx].buy_price  = rules->getBuyCost();
	item_vec[item_idx].sell_price = 0;  // debatable
	item_vec[item_idx].rent_price = rules->getSalaryCost(soul->getRank()); // this is interesting
	item_vec[item_idx].cats = 0;
	item_vec[item_idx].crew_size = 1;
	item_vec[item_idx].in_processing = busy;
	auto soul_idx = item_idx;
	item_idx += 1;
	auto armor_type = soul->getArmor()->getStoreItem();
	if (armor_type != Armor::NONE) {
		item_idx = item_inventory(st, soul->getArmor()->getStoreItem(), 1, item_vec, item_idx, soul_idx, META_ARMOR, true);
	}
	return item_idx;
}
int32_t vehicle_inventory(state_t *st, Vehicle *vehicle, item_t *item_vec, int32_t item_idx, int32_t box_idx, bool busy) {
	auto rule = vehicle->getRules();
	item_vec[item_idx].handle = reinterpret_cast<uintptr_t>(vehicle);
	item_vec[item_idx].box_idx = box_idx;
	item_vec[item_idx].meta = META_VEHICLE;
	item_vec[item_idx].amount = 1;
	item_vec[item_idx].id = st->intern_string(rule->getType());
	item_vec[item_idx].name = st->intern_string(rule->getName());
	item_vec[item_idx].buy_price  = rule->getBuyCost();
	item_vec[item_idx].sell_price  = rule->getSellCost();
	//item_vec[item_idx].rent_price  = rule->getRentCost(); // pity. hmm. how one does this then?
	item_vec[item_idx].ware_size  = rule->getSize();
	item_vec[item_idx].cats = 0;
	item_vec[item_idx].name = st->intern_string(rule->getName());
	item_vec[item_idx].in_processing  = 0; // in a craft.
	item_vec[item_idx].pedia_unlocked  = 0; /// TODO:
	item_vec[item_idx].researchable  = 0;/// TODO:
	item_vec[item_idx].precursor  = 0;/// TODO:
	item_vec[item_idx].in_processing = busy;
	// TODO: ammo.
	auto vehicle_idx = item_idx;
	item_idx += 1;
	return item_idx;
}
int32_t craft_inventory(state_t *st, Base *base, Craft *craft, item_t *item_vec, int32_t item_idx) {
	auto game = st->get_game();
	auto lang = game->getLanguage();
	auto mod = game->getMod();

	auto rule = craft->getRules();
	item_vec[item_idx].handle = reinterpret_cast<uintptr_t>(craft);
	item_vec[item_idx].id = st->intern_string(rule->getType());
	item_vec[item_idx].meta = META_CRAFT;
	item_vec[item_idx].amount = 1;
	item_vec[item_idx].buy_price  = rule->getBuyCost();
	item_vec[item_idx].sell_price  = rule->getSellCost();
	item_vec[item_idx].rent_price  = rule->getRentCost();
	item_vec[item_idx].craft_size  = 1;
	item_vec[item_idx].cats = 0;
	item_vec[item_idx].name = st->intern_string(craft->getName(lang));
	if (craft->getStatus() == "STR_OUT") {
		item_vec[item_idx].box_idx = BOX_OUTSIDE;
		item_vec[item_idx].in_processing  = 1; // flyin' somewhere
	} else {
		item_vec[item_idx].box_idx = BOX_STORES;
		item_vec[item_idx].in_processing  = 0; // repairing? rearming? TODO
	}
	item_vec[item_idx].pedia_unlocked  = 0;
	item_vec[item_idx].researchable  = 0;
	item_vec[item_idx].precursor  = 0;
	auto craft_idx = item_idx;
	item_idx += 1;
	// weapons and ammo
	for (auto wit = craft->getWeapons()->begin(); wit != craft->getWeapons()->end(); ++wit) {
		auto weapon_rules = (*wit)->getRules();
		auto weapon_type = weapon_rules->getLauncherItem();
		auto weapon_idx = item_idx;
		item_idx = item_inventory(st, weapon_type, 1, item_vec, item_idx, craft_idx, META_CRAFT_EQUPMENT, true);
		item_vec[weapon_idx].handle = reinterpret_cast<uintptr_t>(*wit);
		auto clip_type = weapon_rules->getClipItem();
		if (clip_type.length() > 0) { // got ammo!
			auto amount = (*wit)->getClipsLoaded(st->get_game()->getMod());
			item_idx = item_inventory(st, clip_type, amount, item_vec, item_idx, weapon_idx, META_CRAFT_EQUPMENT, true);
		}
	}
	// inventory
	auto craft_items = craft->getItems()->getContents();
	for (auto iit = craft_items->begin(); iit != craft_items->end(); ++iit) {
		item_idx = item_inventory(st, iit->first, iit->second, item_vec, item_idx, craft_idx, META_EQUIPMENT, true);
	}
	// vehicles
	for (auto vit = craft->getVehicles()->begin(); vit != craft->getVehicles()->end(); ++vit) {
		item_idx = vehicle_inventory(st, *vit, item_vec, item_idx, craft_idx, true);
	}
	// crew
	for (auto sit = base->getSoldiers()->begin(); sit != base->getSoldiers()->end(); ++sit) {
		if ((*sit)->getCraft() != craft) {
			continue;
		}
		item_idx = soldier_inventory(st, *sit, item_vec, item_idx, craft_idx, true);
	}
	return item_idx;
}

int32_t get_base_inventory(state_t *st, int32_t base_idx, item_t *item_vec, size_t item_cap) {
	memset(item_vec, 0, sizeof(item_t) * item_cap);
	auto game = st->get_game();
	auto lang = game->getLanguage();
	auto mod = game->getMod();

	if (base_idx < 0 or game->getSavedGame() == NULL) {
		return -1;
	}

	auto bases_vec = game->getSavedGame()->getBases();
	if (bases_vec->size() <= base_idx) {
		return -1;
	}

	auto base = bases_vec->at(base_idx);
	size_t item_idx = 0;
	size_t used_craft = 0, used_barracks = 0, used_ware = 0;
	std::vector<size_t> used_jails;

	{ // craft
		for (auto it = base->getCrafts()->begin(); it != base->getCrafts()->end(); ++it) {
			item_idx = craft_inventory(st, base, *it, item_vec, item_idx);
		}
	}
	{ // vehicles
		for (auto vit = base->getVehicles()->begin(); vit != base->getVehicles()->end(); ++vit) {
			item_idx = vehicle_inventory(st, *vit, item_vec, item_idx, BOX_STORES, false);
		}
	}
	{ // unassigned soldiers
		for (auto sit = base->getSoldiers()->begin(); sit != base->getSoldiers()->end(); ++sit) {
			if ((*sit)->getCraft() != NULL) {
				continue;
			}
			item_idx = soldier_inventory(st, *sit, item_vec, item_idx, BOX_STORES, false);
		}
	}
	{ // engineers
		int32_t idle_engi = base->getEngineers();
		int32_t assigned_engi = 0;
		for (auto it = base->getProductions().cbegin(); it != base->getProductions().end(); ++it) {
			// TODO: make production a container
			assigned_engi += (*it)->getAssignedEngineers();
		}
		if (idle_engi > 0) {
			item_vec[item_idx].meta = META_ENGI;
			item_vec[item_idx].box_idx = BOX_STORES;
			item_vec[item_idx].amount = idle_engi;
			item_vec[item_idx].id = st->intern_string("STR_ENGINEER");
			item_vec[item_idx].buy_price  = mod->getHireEngineerCost();
			item_vec[item_idx].sell_price = 0;
			item_vec[item_idx].rent_price = mod->getEngineerCost();
			item_vec[item_idx].cats = 0;
			item_vec[item_idx].crew_size = 1;
			item_vec[item_idx].in_processing  = 0; // idle
			item_vec[item_idx].pedia_unlocked  = 1; // hmm?
			item_idx += 1;
		}
		if (assigned_engi > 0) {
			item_vec[item_idx].meta = META_ENGI;
			item_vec[item_idx].box_idx = BOX_SHOP_INPUT;
			item_vec[item_idx].amount = assigned_engi;
			item_vec[item_idx].id = st->intern_string("STR_ENGINEER");
			item_vec[item_idx].buy_price  = mod->getHireEngineerCost();
			item_vec[item_idx].sell_price = 0;
			item_vec[item_idx].rent_price = mod->getEngineerCost();
			item_vec[item_idx].cats = 0;
			item_vec[item_idx].crew_size = 1;
			item_vec[item_idx].in_processing  = 1; // working...
			item_vec[item_idx].pedia_unlocked  = 1; // hmm?
			item_idx += 1;
		}
	}
	{ // scientists
		int32_t idle_boffins = base->getScientists();
		int32_t assigned_boffins = 0;
		for (auto it = base->getResearch().cbegin(); it != base->getResearch().end(); ++it) {
			// TODO: make research project a container
			assigned_boffins += (*it)->getAssigned();
		}
		if (idle_boffins > 0) {
			item_vec[item_idx].box_idx = BOX_STORES;
			item_vec[item_idx].meta = META_BOFFIN;
			item_vec[item_idx].amount = idle_boffins;
			item_vec[item_idx].id = st->intern_string("STR_SCIENTIST");
			item_vec[item_idx].buy_price  = mod->getHireScientistCost();
			item_vec[item_idx].sell_price = 0;
			item_vec[item_idx].rent_price = mod->getScientistCost();
			item_vec[item_idx].cats = 0;
			item_vec[item_idx].crew_size = 1;
			item_vec[item_idx].in_processing  = 0;
			item_vec[item_idx].pedia_unlocked  = 1; // hmm?
			item_idx += 1;
		}
		if (assigned_boffins > 0) {
			item_vec[item_idx].box_idx = BOX_LABS;
			item_vec[item_idx].meta = META_BOFFIN;
			item_vec[item_idx].amount = assigned_boffins;
			item_vec[item_idx].id = st->intern_string("STR_SCIENTIST");
			item_vec[item_idx].buy_price  = mod->getHireScientistCost();
			item_vec[item_idx].sell_price = 0;
			item_vec[item_idx].rent_price = mod->getScientistCost();
			item_vec[item_idx].cats = 0;
			item_vec[item_idx].crew_size = 1;
			item_vec[item_idx].in_processing  = 1;
			item_vec[item_idx].pedia_unlocked  = 1; // hmm?
			item_idx += 1;
		}
	}
	{ // transfers
		for (auto it = base->getTransfers()->begin(); it != base->getTransfers()->end(); ++it) {
			switch ((*it)->getType()) {
				case TRANSFER_CRAFT: {
					auto craft_idx = item_idx;
					item_idx = craft_inventory(st, base, (*it)->getCraft(), item_vec, item_idx);
					item_vec[craft_idx].box_idx = BOX_TRANSFERS;
					item_vec[craft_idx].completes_in = (*it)->getHours();
					}
					break;
				case TRANSFER_ITEM: {
					item_vec[item_idx].completes_in = (*it)->getHours();
					item_idx = item_inventory(st, (*it)->getItems(), (*it)->getQuantity(), item_vec, item_idx, BOX_TRANSFERS, META_GENERIC_ITEM, true);
					}
					break;
				case TRANSFER_SOLDIER: {
					auto sol = (*it)->getSoldier();
					auto sol_idx = item_idx;
					item_idx = soldier_inventory(st, sol, item_vec, item_idx, BOX_TRANSFERS, true);
					item_vec[sol_idx].completes_in = (*it)->getHours();
					}
					break;
				case TRANSFER_ENGINEER: {
					item_vec[item_idx].box_idx = BOX_TRANSFERS;
					item_vec[item_idx].meta = META_ENGI;
					item_vec[item_idx].amount = (*it)->getQuantity();
					item_vec[item_idx].completes_in = (*it)->getHours();
					item_vec[item_idx].id = st->intern_string("STR_ENGINEER");
					item_vec[item_idx].buy_price  = mod->getHireEngineerCost();
					item_vec[item_idx].sell_price = 0;
					item_vec[item_idx].rent_price = mod->getEngineerCost();
					item_vec[item_idx].cats = 0;
					item_vec[item_idx].crew_size = 1;
					item_vec[item_idx].in_processing  = 1; // transferring...
					item_vec[item_idx].pedia_unlocked  = 1; // hmm?
					item_idx += 1;
					   }
					break;
				case TRANSFER_SCIENTIST: {
					item_vec[item_idx].box_idx = BOX_TRANSFERS;
					item_vec[item_idx].meta = META_BOFFIN;
					item_vec[item_idx].amount = (*it)->getQuantity();
					item_vec[item_idx].completes_in = (*it)->getHours();
					item_vec[item_idx].id = st->intern_string("STR_SCIENTIST");
					item_vec[item_idx].buy_price  = mod->getHireEngineerCost();
					item_vec[item_idx].sell_price = 0;
					item_vec[item_idx].rent_price = mod->getEngineerCost();
					item_vec[item_idx].cats = 0;
					item_vec[item_idx].crew_size = 1;
					item_vec[item_idx].in_processing  = 1; // transferring...
					item_vec[item_idx].pedia_unlocked  = 1; // hmm?
					item_idx += 1;
					}
					break;
				default:
					break;
			}
		}
	}
	{ // stores
		auto base_storage = base->getStorageItems()->getContents();
		for (auto it = base_storage->begin(); it != base_storage->end(); ++it) {
			item_idx = item_inventory(st, it->first, it->second, item_vec, item_idx, BOX_STORES, META_GENERIC_ITEM, false);
		}
	}

	return item_idx;
}
int32_t adjust_funds(state_t *st, int32_t amount) {
	auto sgame = st->get_game()->getSavedGame();
	sgame->setFunds(sgame->getFunds() + amount);
	return sgame->getFunds();
}
static double interbase_distance(Base *a, Base *b) {
	double x[3], y[3], z[3], r = 51.2;
	Base *base = a;
	for (int i = 0; i < 2; ++i) {
		x[i] = r * cos(base->getLatitude()) * cos(base->getLongitude());
		y[i] = r * cos(base->getLatitude()) * sin(base->getLongitude());
		z[i] = r * -sin(base->getLatitude());
		base = b;
	}
	x[2] = x[1] - x[0];
	y[2] = y[1] - y[0];
	z[2] = z[1] - z[0];
	return sqrt(x[2] * x[2] + y[2] * y[2] + z[2] * z[2]);
}
int32_t transfer_cost(uintptr_t base_a, uintptr_t base_b, int32_t multiplier) {
	auto a_ptr = reinterpret_cast<Base *>(base_a);
	auto b_ptr = reinterpret_cast<Base *>(base_a);
	return (int)(multiplier * interbase_distance(a_ptr, b_ptr));
}
int32_t buy_items(state_t *st, uintptr_t base_handle, const char *item_type, int32_t amount, int32_t unit_buy_price, int32_t time) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto t = new Transfer(time);
	t->setItems(item_type, amount);
	base->getTransfers()->push_back(t);
	adjust_funds(st, -unit_buy_price  * amount);
	return 0;
}
int32_t buy_craft(state_t *st, uintptr_t base_handle, const char *craft_type, int32_t amount, int32_t unit_buy_price, int32_t time) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto rule = st->get_game()->getMod()->getCraft(craft_type, false);
	if (rule == NULL) {
		Log(LOG_ERROR) << "Can't buy unknown craft type '" << craft_type << "'";
		return -1;
	}
	for (int c = 0; c < amount; c++) {
		auto craft = new Craft(rule, base, 0); // id? hmm.
		auto t = new Transfer(time);
		t->setCraft(craft);
		base->getTransfers()->push_back(t);
	}
	adjust_funds(st, -unit_buy_price * amount);
	return 0;
}
int32_t hire_crew(state_t *st, uintptr_t base_handle, const char *crew_type, int32_t amount, int32_t unit_buy_price, int32_t time) {
	auto base  = reinterpret_cast<Base *>(base_handle);
	auto game  = st->get_game();
	auto sgame = game->getSavedGame();
	auto mod   = game->getMod();
	auto rule  = mod->getSoldier(crew_type, false);
	if (rule == NULL) {
		Log(LOG_ERROR) << "Can't hire unknown soldier type '" << crew_type << "'";
		return -1;
	}
	for (int c = 0; c < amount; c++) {
		auto t = new Transfer(time);
		t->setSoldier(mod->genSoldier(sgame, crew_type));
		base->getTransfers()->push_back(t);
	}
	adjust_funds(st, -unit_buy_price  * amount);
	return 0;
}
int32_t hire_engi(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_buy_price, int32_t time) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto t = new Transfer(time);
	t->setEngineers(amount);
	base->getTransfers()->push_back(t);
	adjust_funds(st, -unit_buy_price  * amount);
	return 0;
}
int32_t hire_boffins(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_buy_price, int32_t time) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto t = new Transfer(time);
	t->setScientists(amount);
	base->getTransfers()->push_back(t);
	adjust_funds(st, -unit_buy_price  * amount);
	return 0;
}
int32_t sell_items(state_t *st, uintptr_t base_handle, const char *item_type, int32_t amount, int32_t unit_sale_price) {
	auto base = reinterpret_cast<Base *>(base_handle);
	if (base->getStorageItems()->getItem(item_type) > amount) {
		base->getStorageItems()->removeItem(item_type, amount);
		adjust_funds(st, unit_sale_price * amount);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to sell more of '" << item_type << "' than there is.";
	return -1;
}
int32_t sell_craft(state_t *st, uintptr_t base_handle, uintptr_t handle, int32_t unit_sale_price) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto craft = reinterpret_cast<Craft *>(handle);
	bool done = false;

	for (auto c = base->getCrafts()->begin(); c != base->getCrafts()->end(); ++c) {
		if (*c == craft) {
			done = true;
			craft->unload(st->get_game()->getMod());
			for (auto f = base->getFacilities()->begin(); f != base->getFacilities()->end(); ++f) {
				if ((*f)->getCraftForDrawing() == craft) {
					(*f)->setCraftForDrawing(0);
					break;
				}
			}
			base->getCrafts()->erase(c);
			delete craft;
			break;
		}
	}

	if (done) {
		adjust_funds(st, unit_sale_price);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to sell a craft that is not there.";
	return -1;
}
int32_t sack_crew(state_t *st, uintptr_t base_handle, uintptr_t handle, int32_t unit_sale_price) {
	auto base = reinterpret_cast<Base *>(base_handle);
	auto soul = reinterpret_cast<Soldier *>(handle);
	bool done = false;
	for (auto s = base->getSoldiers()->begin(); s != base->getSoldiers()->end(); ++s) {
		if (*s == soul) {
			if ((*s)->getArmor()->getStoreItem() != Armor::NONE) {
				base->getStorageItems()->addItem((*s)->getArmor()->getStoreItem());
			}
			base->getSoldiers()->erase(s);
			done = true;
			delete soul;
			break;
		}
	}
	if (done) {
		adjust_funds(st, unit_sale_price);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to sack crew that is not there.";
	return -1;
}
int32_t sack_engi(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_sale_price) {
	auto base = reinterpret_cast<Base *>(base_handle);
	if (base->getEngineers() >= amount) {
		base->setEngineers(base->getEngineers() - amount);
		adjust_funds(st, amount * unit_sale_price);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to sack engi that are not there.";
	return -1;
}
int32_t sack_boffins(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_sale_price) {
	auto base = reinterpret_cast<Base *>(base_handle);
	if (base->getScientists() >= amount) {
		base->setScientists(base->getScientists() - amount);
		adjust_funds(st, amount * unit_sale_price);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to sack boffins that are not there.";
	return -1;
}
int32_t transfer_items(state_t *st, uintptr_t base_from, uintptr_t base_to, const char *item_type, int32_t amount, int32_t time, int32_t unit_cost) {
	auto b_from = reinterpret_cast<Base *>(base_from);
	auto b_to = reinterpret_cast<Base *>(base_to);
	bool done = false;
	if (b_from->getStorageItems()->getItem(item_type) > amount) {
		b_from->getStorageItems()->removeItem(item_type, amount);
		auto t = new Transfer(time);
		t->setItems(item_type, amount);
		b_to->getTransfers()->push_back(t);
		done = true;
	}
	if (done) {
		adjust_funds(st, unit_cost * amount);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to transfer more of '" << item_type << "' than there is.";
	return -1;
}
/* giant TODO: IF the craft has enough range and IF it has enough pilots, and maybe IF it would be faster than just setting up a transfer
 * THEN just launch it, change home base and set it to RTB
 */
int32_t transfer_craft(state_t *st, uintptr_t base_from, uintptr_t base_to, uintptr_t handle, int32_t time, int32_t cost) {
	auto b_from = reinterpret_cast<Base *>(base_from);
	auto b_to = reinterpret_cast<Base *>(base_to);
	auto craft = reinterpret_cast<Craft *>(handle);
	bool done = false;
	for (auto craft_it = b_from->getCrafts()->begin(); craft_it != b_from->getCrafts()->end(); ++craft_it) {
		if (*craft_it == craft) {
			if (craft->getStatus() == "STR_OUT") {
				bool returning = (craft->getDestination() == (Target*)craft->getBase());
				b_to->getCrafts()->push_back(craft);
				craft->setBase(b_to, false);
				if (craft->getFuel() <= craft->getFuelLimit(b_to)) {
					craft->setLowFuel(true);
					craft->returnToBase();
				} else if (returning) {
					craft->setLowFuel(false);
					craft->returnToBase();
				}
			} else {
				auto t = new Transfer(time);
				t->setCraft(*craft_it);
				b_to->getTransfers()->push_back(t);
			}
			// TODO: this should be a method in Base. rearrange_craft() or something.
			for (auto fac_it = b_to->getFacilities()->begin(); fac_it != b_to->getFacilities()->end(); ++fac_it) {
				if ((*fac_it)->getCraftForDrawing() == *craft_it) {
					(*fac_it)->setCraftForDrawing(0);
					break;
				}
			}
			for (auto soul_it = b_from->getSoldiers()->begin(); soul_it != b_from->getSoldiers()->end(); ++soul_it) {
				auto soul = *soul_it;
				if (soul->getCraft() == craft) {
					if (soul->isInPsiTraining()) {
						soul->setPsiTraining();
					}
					soul->setTraining(false);
					if (craft->getStatus() == "STR_OUT") {
						b_to->getSoldiers()->push_back(soul);
					} else {
						auto t = new Transfer(time);
						t->setSoldier(soul);
						b_to->getTransfers()->push_back(t);
					}
					b_from->getSoldiers()->erase(soul_it);
				}
			}
			b_from->getCrafts()->erase(craft_it);
			done = true;
			break;
		}
	}
	if (done) {
		// TODO: shouldn't we include soldier transfer costs when sendin'em by transfer?
		adjust_funds(st, cost);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to transfer craft that is not there.";
	return -1;
}
int32_t transfer_crew(state_t *st, uintptr_t base_from, uintptr_t base_to, uintptr_t handle, int32_t time, int32_t cost) {
	auto b_from = reinterpret_cast<Base *>(base_from);
	auto b_to = reinterpret_cast<Base *>(base_to);
	auto soul = reinterpret_cast<Soldier *>(handle);
	bool done = false;
	for (auto s = b_from->getSoldiers()->begin(); s != b_from->getSoldiers()->end(); ++s) {
		if (*s == soul) {
			if (soul->isInPsiTraining()) {
				soul->setPsiTraining();
			}
			soul->setTraining(false);
			auto t = new Transfer(time);
			t->setSoldier(soul);
			b_to->getTransfers()->push_back(t);
			b_from->getSoldiers()->erase(s);
			done = true;
			break;
		}
	}
	if (done) {
		adjust_funds(st, cost);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to transfer crew that is not there.";
	return -1;
}
int32_t transfer_engi(state_t *st, uintptr_t base_from, uintptr_t base_to, int32_t amount, int32_t time, int32_t unit_cost) {
	auto b_from = reinterpret_cast<Base *>(base_from);
	auto b_to = reinterpret_cast<Base *>(base_to);
	if (b_from->getEngineers() >= amount) {
		b_from->setEngineers(b_from->getEngineers() - amount);
		auto t = new Transfer(time);
		t->setEngineers(amount);
		b_to->getTransfers()->push_back(t);
		adjust_funds(st, unit_cost * amount);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to transfer boffins that are not there.";
	return -1;
}
int32_t transfer_boffins(state_t *st, uintptr_t base_from, uintptr_t base_to, int32_t amount, int32_t time, int32_t unit_cost) {
	auto b_from = reinterpret_cast<Base *>(base_from);
	auto b_to = reinterpret_cast<Base *>(base_to);
	if (b_from->getScientists() >= amount) {
		b_from->setScientists(b_from->getScientists() - amount);
		auto t = new Transfer(time);
		t->setScientists(amount);
		b_to->getTransfers()->push_back(t);
		adjust_funds(st, unit_cost * amount);
		return 0;
	}
	Log(LOG_ERROR) << "Attempting to transfer boffins that are not there.";
	return -1;
}
int32_t research_stuff(state_t *st,  uintptr_t base_handle, const char *item_type, int32_t workforce) {
	return -1;
}

//}
