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

#include "module.h" // for the action_t
#include "adapter.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/CraftWeapon.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Armor.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/State.h"
#include "../Engine/Logger.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ArrowButton.h"
#include "../Savegame/Production.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"

using namespace OpenXcom;

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
//{ ui
struct _state_t : public State {
	Window *_window;
	std::string _id, _category;
	std::vector<std::string *> _interned_strings;

	_state_t(State *parent, int32_t w, int32_t h, int32_t x, int32_t y, WindowPopup anim_type,
			 const char *ui_category, const char *background) {

		_screen = false; // wtf is this, aha, it's if it replaces the whole screen. ok.

		_window = new Window(this, w, h, x, y, anim_type);

		// here go elements' constuctors in the openxcom code.

		// "geoscape" is the ui_category
		const bool alterPal = false;
		setInterface(ui_category, alterPal, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);

		auto bg_surface = _game->getMod()->getSurface(background);
		if (bg_surface)
			_window->setBackground(bg_surface);
		else
			Log(LOG_ERROR) << "_state_t(): failed to get background surface" << background;

		add(_window, "window", ui_category);
	}
	~_state_t() { for (auto is: _interned_strings) { delete is; } }

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

	Window *get_window() { return _window; }
	void toggle_screen() { toggleScreen(); }

	// get game
	Game *get_game() { return _game; }

	// keep a string for the lifetime of the state
	const char *intern_string(const std::string& s) {
		auto *is = new std::string(s);
		_interned_strings.push_back(is);
		return is->c_str();
	}

	// same for the disgusting wstrings
	const char *intern_string(const std::wstring &ws) {
		return intern_string(Language::wstrToUtf8(ws));
	}
};

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

void *st_get_window(state_t *st) {
	return st->get_window();
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

// enum ArrowOrientation { ARROW_VERTICAL, ARROW_HORIZONTAL };   0, 1


struct _textlist_t : public TextList {
	_textlist_t(int w, int32_t h, int32_t x, int32_t y) : TextList(w, h, x, y) { }

	void onLeftArrowClick(Action *a) { }
	void onRightArrowClick(Action *a) { }
};
textlist_t *st_add_text_list(state_t *st, int32_t w, int32_t h, int32_t x, int32_t y,
								 const char *ui_element, const char *ui_category) {

	  auto rv = new textlist_t(w, h, x, y);
	  st->add(rv, ui_element, ui_category);
	  return rv;
}
void textlist_add_row(textlist_t *tl, int cols, ...)  {
	va_list args;
	va_start(args, cols);
	tl->vAddRow(cols, args);
	va_end(args);
}
void textlist_set_columns(textlist_t *tl, int cols, ...)  {
	va_list args;
	va_start(args, cols);
	tl->vSetColumns(cols, args);
	va_end(args);
}
void textlist_set_selectable(textlist_t *tl, bool flag)  {\
	tl->setSelectable(flag);
}
void textlist_set_arrow_column(textlist_t *tl, int pos, int orientation)  {
	tl->setArrowColumn(pos, orientation ? ARROW_HORIZONTAL : ARROW_VERTICAL);
}
void textlist_set_background(textlist_t *tl, void *someptr)  {
	tl->setBackground(static_cast<Surface *>(someptr));
}
void textlist_set_margin(textlist_t *tl, int margin)  {
	tl->setMargin(margin);
}

//} ui
//{ translations
// just so we have some lifetime to the values returned by st_translate().
std::unordered_map<std::string, std::string> translations;
void pypy_reset_translations(void) {
	translations.clear();
}
/* get STR_whatever translations in utf8 */
const char *st_translate(state_t *st, const char *key) {
	auto i = translations.find(key);
	if (i == translations.end()) {
		auto trans = st->tr(key);
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

void prepare_static_mod_data(uintptr_t _game) {
	Game *game = reinterpret_cast<Game *>(_game);
	static_craft_weapon_types.clear();
	static_armor_types.clear();
	static_item_categories.clear();
	static_precursor_items.clear();

	auto& cw = game->getMod()->getCraftWeaponsList();
	for (auto i = cw.begin(); i != cw.end(); ++i)
	{
		RuleCraftWeapon *rule = game->getMod()->getCraftWeapon(*i);
		static_craft_weapon_types.insert(rule->getLauncherItem());
		static_craft_weapon_types.insert(rule->getClipItem());
	}
	auto& ar = game->getMod()->getArmorsList();
	for (auto i = ar.begin(); i != ar.end(); ++i)
	{
		Armor *rule = game->getMod()->getArmor(*i);
		static_armor_types.insert(rule->getStoreItem());
	}
	auto& cats = game->getMod()->getItemCategoriesList();
	for (auto i = cats.cbegin(); i != cats.cend(); ++i) {
		static_item_categories.insert(*i);
	}
	auto& manu_projects = game->getMod()->getManufactureList();
	for (auto mpi = manu_projects.cbegin(); mpi != manu_projects.cend(); ++mpi ) {
		auto rule = game->getMod()->getManufacture(*mpi);
		auto& reqs = rule->getRequiredItems();
		for (auto req = reqs.begin(); req != reqs.end(); ++req) {
			static_precursor_items.insert((*req).first->getType());
		}
	}
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
