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

#pragma once
#include <stdint.h>

/* here goes the api that's exposed via cffi to something */

#ifdef __cplusplus
extern "C" {
#else
typedef uint8_t bool;
#endif

void set_game_ptr(void *g);

/* UI */
typedef struct _state_t state_t;

void *state_not_found(const char *name);
state_t *new_state(int32_t w, int32_t h, int32_t x, int32_t y, const char *ui_id, const char *ui_category);
void st_clear(state_t *state);
void st_fill(state_t *state, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void st_blit(state_t *state, int32_t dst_x, int32_t dst_y, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h, uintptr_t upsrc);
uintptr_t st_intern_sub(state_t *state, const char *name, int32_t idx);
uintptr_t st_intern_surface(state_t *state, const char *name);
uintptr_t st_intern_text(state_t *state, int32_t w, int32_t h, const char *text, int32_t halign, int32_t valign, int32_t wrap,
		int32_t primary, int32_t secondary, int32_t is_small);
void st_play_ui_sound(state_t *state, const char *name);
const char* st_translate(state_t* state, const char* key);
void drop_translations(void);

// stuff to be used from the loaded_game / saving_game hooks
const char *get_interface_rules();  	// this one spits out a complete 'interfaces' map in yaml, direct from Mod.

/* log levels:
 *    LOG_FATAL = 0
 *    LOG_ERROR
 *    LOG_WARNING
 *    LOG_INFO
 *    LOG_DEBUG
 *    LOG_VERBOSE
 * message must be utf8
 */
void logg(int level, const char *message);

/* Game data */

typedef struct _base_t {
	uintptr_t handle;
	int32_t idx;
	const char *name;			// NULL = end of list/no game loaded.
	int32_t facility_count;		// exact number of facilities in the base
	int32_t inventory_count;	// ceiling on item count in the base
} base_t;

typedef struct _bat_cat_t {
	const char *id;
	uint64_t mask;
} item_category_t;

typedef struct _facility_t {
	uintptr_t handle;
	const char *id;				// STR_whatever. NULL for no such facility/invalid base num etc.
	int32_t online;
	int32_t ware_capacity;		// vaults
	int32_t crew_capacity;		// barracks
	int32_t craft_capacity;		// hangars
	int32_t jail_capacity;		// prisons
	int32_t jail_type;
	int32_t maintenance;		// monthly cost
} facility_t;

/* Base inventory: the hierarchy:
	- just what's in the base at the moment. box_idx = -1.
	- craft:
		- craft weapons         	: craft's box_idx
		- craft weapons' ammo		: craft weapon's box_idx
		- items						: craft's box_idx
		- assigned crew				: craft's box_idx
		- assigned vehicles			: craft's box_idx
		- assigned vehicles' ammo 	: vehicle's box_idx
	- unassigned scientists
	- unassigned engineers
	- unassigned crew
	- items in stores
	- prisoners in jails
	- researched: box_idx = -2
	- items researched
	- prisoners researched
	- researchers assigned
	?- craft researched? not possible atm.
	- manufacturing supply: box_idx = -3
	- engineers assigned
	- craft manufactured: should consume a hangar but how? dere's no Craft instance for this. Okay, we'll synth.
	?- precursor items for manufacturing (not possible now, they are consumed right away)
	- manufacturing output (single item per job): box_idx = -4
	- item
	- craft out of base (craft->getState() == STR_OUT) : box_idx = -5
	- craft:
		- craft weapons         	: craft's box_idx
		- craft weapons' ammo		: craft weapon's box_idx
		- items						: craft's box_idx
		- assigned crew				: craft's box_idx
		- assigned vehicles			: craft's box_idx
		- assigned vehicles' ammo 	: vehicle's box_idx
	- transfers: box_idx = -6
	- crew
	- researchers
	- scientists
	- engineers
	- crew
	- items
	- craft:
		- craft weapons         	: craft's box_idx
		- craft weapons' ammo		: craft weapon's box_idx
		- items						: craft's box_idx
		- assigned crew				: craft's box_idx
		- assigned vehicles			: craft's box_idx
		- assigned vehicles' ammo 	: vehicle's box_idx
*/


typedef enum _item_meta_type {
	META_GENERIC_ITEM = 0,	// default in case I forgot something
	META_COMPONENTS,       	// like, alien alloys
	META_EQUIPMENT,			// what gets loaded onto the craft?
	META_ARMOR,				// armor kinda.
	META_CRAFT_EQUPMENT,	// guns and stuff and also ammo
	META_CRAFT,				// craft, eh
	META_VEHICLE,			// tanks, maybe. only those loaded in teh craft for now. TODO.
	META_CREW,				// soldier
	META_ENGI,				// engineer
	META_BOFFIN,			// scientist
	META_PRISONER,			// jailbird
	META_RESEARCH,			// research project
	META_MANUFACTURE		// manufacturing project
} item_meta_type_t;

typedef enum _inventory_box_type { // this is special box_idx values - pseudo containers
	BOX_STORES 			= -1,
	BOX_LABS 			= -2,
	BOX_SHOP_INPUT 		= -3,
	BOX_SHOP_OUTPUT		= -4,
	BOX_OUTSIDE 		= -5,
	BOX_TRANSFERS 		= -6,
} inventory_box_type_t;

typedef enum _researchablessness_type {
	RLNT_DONE				= 0, 	// no indicator <- also means ufopaedia should be available for this.
	RLNT_RIGHT_NOW 			= 1,  	// 'R'
	RLNT_MISSES_FACILITY 	= 2,	// 'f' (dependencies are met, only facility is missing)
	RLNT_MISSES_DEPENDENCY	= 3,	// 'p' (and maybe facility too)
} researchablessness_type_t;

typedef struct _item_t {
	uintptr_t	handle;			// pointer to Soldier, Craft, CraftWeapon, ResearchProject, etc.
	int32_t		meta;			// meta type
	int32_t     box_idx;		// array index for the containing container or specbox
								// because must unequip to transfer/sell/etc
	int32_t     amount;			// the whole point of this
	int32_t		completes_in;	// transfer, repair, reload, manufacturing time left, hours; (inf is 0 for manuf)
	bool 		in_processing;	// being researched or working in lab/workshop, assigned to a craft, in transfer, etc.
	bool 		pedia_unlocked;	// pedia article accessible. Hmm. Should be const char * to the pedia article to spawn the state
	int32_t		researchable;	// some unresearched research depends on this - RLNT_*
	bool 		precursor;		// some manufacture depends on this
	bool 		autosell;		// if it gets autosold
	// the rest depends on the ruleset, not the current state
	const char *id;				// STR_whatever
	uint64_t    cats;			// cats. cats are nice.
	const char *name; 			// craft name if this is a craft, or the alternate item name.
	int32_t		ware_size;		// what it takes to store..
	int32_t 	crew_size;		// >1 == too fat ? :)
	int32_t 	craft_size;		// basically 0 or 1. HMM. but whatever. let it be. maybe we store dinosaur skeletons in hangars
	int32_t 	jail_size; 		// idk why reapers take only one jail cell
	int32_t 	jail_type;		// https://openxcom.org/forum/index.php/topic,4830.msg69933.html#msg69933
	int32_t 	buy_price;
	int32_t 	rent_price;
	int32_t 	sell_price;
} item_t;

// good old C - fac_vec is expected to hold at least fac_cap structs.
// we can return ptr-to-prt to a buffer of structs internally allocated as std::vector
// and then interned, but memory consumption then depends on usage patterns
// since it can only be freed when the state is freed, up until that it just grows.
// on the other hand... for the base inventory mgmt that might be acceptable.
// can't do finegrained flush, but if we're for example reinitializing the whole state anyway...
// returned values' lifetime is at most the lifetime of a state (ui window).
// and they are not to be modified.

// just iterate over base_idx until you get -1 as rv. Returns the index of the base otherwise.
int32_t get_base_facilities(state_t *st, int32_t base_idx, base_t *base, facility_t *fac_vec, size_t fac_cap);
// same here. returns number of items written or -1 in case of no such base.
int32_t get_base_inventory(state_t *st, int32_t base_idx, item_t *item_vec, size_t item_cap);
// same here.
//int32_t get_item_categories(state_t *st, item_category_t *cat_vec, size_t cat_cap);
/* vanilla transfer cost multipliers; not moddable:
	- Soldier:		 5
	- Craft:		25
	- Scientist:	 5
	- Engineer:		 5
	- Item:			 1

	Multiplier multiplies straight line (not great arc) distance between bases on a sphere of R=51.2 (don't ask...) */
int32_t transfer_cost(uintptr_t base_a, uintptr_t base_b, int32_t multiplier);

// the meat of the thing
int32_t buy_items(state_t *st, uintptr_t base_handle, const char *item_type, int32_t amount, int32_t unit_buy_price, int32_t time);
int32_t buy_craft(state_t *st, uintptr_t base_handle, const char *craft_type, int32_t amount, int32_t unit_buy_price, int32_t time);
int32_t hire_crew(state_t *st, uintptr_t base_handle, const char *crew_type, int32_t amount, int32_t unit_buy_price, int32_t time);
int32_t hire_engi(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_buy_price, int32_t time);
int32_t hire_boffins(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_buy_price, int32_t time);

int32_t sell_items(state_t *st, uintptr_t base_handle, const char *item_type, int32_t amount, int32_t unit_sale_price);
int32_t sell_craft(state_t *st, uintptr_t base_handle, uintptr_t handle, int32_t unit_sale_price);
int32_t sack_crew(state_t *st, uintptr_t base_handle, uintptr_t handle, int32_t unit_sale_price);
int32_t sack_engi(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_sale_price);
int32_t sack_boffins(state_t *st, uintptr_t base_handle, int32_t amount, int32_t unit_sale_price);

int32_t transfer_items(state_t *st, uintptr_t base_from, uintptr_t base_to, const char *item_type, int32_t amount, int32_t time, int32_t unit_cost);
int32_t transfer_craft(state_t *st, uintptr_t base_from, uintptr_t base_to, uintptr_t handle, int32_t time, int32_t cost);
int32_t transfer_crew(state_t *st, uintptr_t base_from, uintptr_t base_to, uintptr_t handle, int32_t time, int32_t cost);
int32_t transfer_engi(state_t *st, uintptr_t base_from, uintptr_t base_to, int32_t amount, int32_t time, int32_t unit_cost);
int32_t transfer_boffins(state_t *st, uintptr_t base_from, uintptr_t base_to, int32_t amount, int32_t time, int32_t unit_cost);

int32_t research_stuff(state_t *st,  uintptr_t base_handle, const char *item_type, int32_t workforce);

// load various sets and stuff. unconditionally called at game_loaded.
// TODO: split it into  at_mod_load, at_game_load
void prepare_static_mod_data(); // see Mod::resetGlobalStatics()

/* Geoscape modding approaches:

	- Custom UFOs and their strategies:
		- design an api on the Target level:
		    - get existing oxce-driven targets: bases, abases, craft, ufos
			- spawn/remove
			- set visibile/invisible
			- notify of detection
			- move
			- calculate distances
			- handle click -> open UI corresponding to.
		- interface to existing dogfight UI:
			- custom ufos that are fully defined in yaml
			- set strategies: hold off, aggressive, etc.
			- handle ticks
		- custom dogfight UI - well, write it
		- interface to the battlescape mission generator:
			- for a start just mimic default behavior  vv

*/
#ifdef __cplusplus
} // extern "C" - this comment is required due to pypy/cffi build system
#endif
