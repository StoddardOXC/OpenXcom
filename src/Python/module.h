/*
# Copyright 2017,2018 Stoddard, https://github.com/StoddardOXC.
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFFI_DLLEXPORT
#  if defined(_MSC_VER)
#    define CFFI_DLLEXPORT  extern __declspec(dllimport)
#  else
#    define CFFI_DLLEXPORT  extern
#  endif
#endif

typedef struct _pypy_syspaths_t {
	const char *python_dir;
	const char *cfg_dir;
	const char *user_dir;
	const char *data_dir;
} pypy_syspaths_t;

typedef struct _pypy_dumb_buf_t {
	const char *ptr;
	int32_t size;
} pypy_dumb_buf_t;

typedef struct _state_t state_t;

CFFI_DLLEXPORT void pypy_initialize(void *);
CFFI_DLLEXPORT void pypy_set_modpath(const char *);
CFFI_DLLEXPORT void pypy_hook_up(const char *);
CFFI_DLLEXPORT void pypy_reset_mods_statics();
CFFI_DLLEXPORT void pypy_mods_loaded();
/* ui hooks */
CFFI_DLLEXPORT void *pypy_spawn_state(const char *state_name);
CFFI_DLLEXPORT void pypy_forget_state(void *);
CFFI_DLLEXPORT void pypy_state_input(void *state);
CFFI_DLLEXPORT void pypy_state_blit(void *state);
CFFI_DLLEXPORT void pypy_state_think(void *state);

/* non-overriding hooks */
CFFI_DLLEXPORT void pypy_lang_changed();
CFFI_DLLEXPORT void pypy_game_loaded(const char *pypy_data, int32_t data_size);
CFFI_DLLEXPORT void pypy_saving_game(pypy_dumb_buf_t *buf);
CFFI_DLLEXPORT void pypy_game_saved();
CFFI_DLLEXPORT void pypy_game_abandoned();

CFFI_DLLEXPORT void pypy_time_5sec();
CFFI_DLLEXPORT void pypy_time_10min();
CFFI_DLLEXPORT void pypy_time_30min();
CFFI_DLLEXPORT void pypy_time_1hour();
CFFI_DLLEXPORT void pypy_time_1day();
CFFI_DLLEXPORT void pypy_time_1mon();

/* overriding hooks: return value: nonzero if orig code is supposed to run. */
CFFI_DLLEXPORT int32_t pypy_battle_ended();
CFFI_DLLEXPORT int32_t pypy_craft_clicked(void *craft, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_ufo_clicked(void *ufo, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_base_clicked(void *base, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_alienbase_clicked(void *abase, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_craft_shot_down(void *craft);
CFFI_DLLEXPORT int32_t pypy_craft_at_target(void *craft, void *target);
CFFI_DLLEXPORT int32_t pypy_transfer_done(void *transfer);
CFFI_DLLEXPORT int32_t pypy_research_done(void *research);
CFFI_DLLEXPORT int32_t pypy_manufacture_done(void *manufacture);
/*
CFFI_DLLEXPORT int32_t pypy_(, );
*/

#ifdef __cplusplus
} // extern "C" - this
#endif
