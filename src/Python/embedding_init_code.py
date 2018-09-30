#
# Copyright 2017 Stoddard, https://github.com/StoddardOXC.
#
# This file is a part of OpenXcom.
#
# OpenXcom is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenXcom is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
#

import sys, os, os.path, importlib, inspect, traceback
from pypycom import ffi, lib

LOG_FATAL   = 0
LOG_ERROR   = 1
LOG_WARNING = 2
LOG_INFO    = 3
LOG_DEBUG   = 4
LOG_VERBOSE = 5

def _log_encode(*args):
    t = ''.join(str(a) for a in args)
    t = t.encode('utf-8')
    return t
def log_fatal(*args):
    lib.logg(LOG_FATAL, _log_encode(*args))
def log_error(*args):
    lib.logg(LOG_ERROR, _log_encode(*args))
def log_warning(*args):
    lib.logg(LOG_WARNING, _log_encode(*args))
def log_info(*args):
    lib.logg(LOG_INFO, _log_encode(*args))
def log_debug(*args):
    lib.logg(LOG_DEBUG, _log_encode(*args))
def log_verbose(*args):
    lib.logg(LOG_VERBOSE, _log_encode(*args))
def log_exception(exc_type, exc_value, exc_traceback):
    for l in traceback.format_exception(exc_type, exc_value, exc_traceback):
        log_error("PyPy: {}".format(l)[:-1])
    log_fatal("PyPy code threw an exception: fatal.")

def ptr2int(ptr):
    return int(ffi.cast("uintptr_t", ptr))

cfg_dir = None
user_dir = None
python_dir = None

state_types = {}
state_instances = {}
hooks = {}

# default hooks:

def game_abandoned(game):
    pass
def game_loaded(game, pod): # pod = Plain Old Data: something deserealized from yaml.
    pass
def saving_game(game, pod):
    pass
def clock_tick(game, step):
    pass

def battle_ended(game):
    return 1
def craft_shot_down(game, craft):
    return 1
def craft_at_target(game, craft, target):
    return 1
def alienbase_clicked(game, base, button, kmod):
    return 1
def base_clicked(game, base, button, kmod):
    return 1
def ufo_clicked(game, ufo, button, kmod):
    return 1
def craft_clicked(game, craft, button, kmod):
    return 1

def transfer_done(game, transfer):
    return 1
def research_done(game, research):
    return 1
def manufacture_done(game, manufacture):
    return 1

def import_modules(drop_old = False):
    global state_types, python_dir, hooks
    if drop_old:
        state_types = {}
        hooks = {}
    for pname in os.listdir(python_dir):
        if  pname not in ('api.py', '__pycache__') and (pname.endswith('.py') or os.path.isdir(os.path.join(python_dir, pname))):
            modname = pname
            if pname.endswith('.py'):
                modname = pname[:-3]
            elif os.path.isdir(os.path.join(python_dir, pname)):
                pname += '/'
            log_info("import {} ({})".format(modname, pname))
            try:
                mod = importlib.import_module(modname)
                got_stuff = set()
                try:
                    mod_states = mod.__states__
                except:
                    mod_states = {}
                try:
                    mod_hooks = mod.__hooks__
                except:
                    mod_hooks = {}
                if len(mod_states) + len(mod_hooks) == 0:
                    log_info("    mod {}: no __states__ or __hooks__".format(modname))

                for memtuple in inspect.getmembers(mod):
                    memname, member = memtuple
                    if memname in mod_states:
                        state_types[memname] = member
                        got_stuff.add(memname)
                    elif memname in mod_hooks:
                        hooks[memname] = member
                        got_stuff.add(memname)
                log_info("    imported {} [{}]".format(modname, ', '.join(got_stuff)))
            except Exception as e:
                log_fatal("    mod ", modname, ": ", e)
                raise

@ffi.def_extern(onerror = log_exception)
def pypy_initialize(p):
    global user_dir, cfg_dir, python_dir, data_dir, state_types
    cfg_dir = os.path.realpath(ffi.string(p.cfg_dir).decode('utf-8'))
    data_dir = os.path.realpath(ffi.string(p.data_dir).decode('utf-8'))
    user_dir = os.path.realpath(ffi.string(p.user_dir).decode('utf-8'))
    python_dir = os.path.realpath(ffi.string(p.python_dir).decode('utf-8'))
    sys.path.append(python_dir)
    log_info("PyPy cfg_dir: ", cfg_dir)
    log_info("PyPy data_dir: ", cfg_dir)
    log_info("PyPy user_dir: ", user_dir)
    log_info("PyPy python_dir: ", python_dir)
    log_info("PyPy sys.path:")
    for p in sys.path:
        log_info("    ", p)
    import_modules(drop_old = False)
    log_info("PyPy states:")
    for stname in state_types.keys():
        log_info("    ", stname)
    log_info("PyPy hooks:")
    for hname in hooks.keys():
        log_info("    ", hname)
    log_info("PyPy initialized.")
    return 42

# What happens is that this constructs a pypy state object by class name.
#
# State class' base class (State) constructor calls lib.new_state(),
# which constructs a C++ object derived from oxce's State class.
#
# Then the constructed pypy state object inherits the ptr property,
# which basically returns C++ object's address, which is used as key
# in a dict used to dispatch events coming up from the oxce engine
# such as input events and blit requests.
#
# A game frame is processed as follows:
#
# - all input events are converted to Action-s and fed to the topmost State only.
# - topmost State's think() method is called.
# - all States' blit() method is called.
#
#
@ffi.def_extern(onerror = log_exception)
def pypy_spawn_state(name):
    global state_types, state_instances
    name = ffi.string(name).decode('utf-8')
    try:
        stclass = state_types[name]
    except KeyError:
        log_error("pypy_spawn_state({}): state class not found".format(name))
        return ffi.NULL
    newstate = state_types[name]()
    state_id = ptr2int(newstate.ptr)
    state_instances[state_id] = newstate
    log_info("pypy_spawn_state({}): returning {} for {}".format(name, newstate.ptr, newstate))
    return newstate.ptr

@ffi.def_extern(onerror = log_exception)
def pypy_state_input(state_ptr):
    global state_types, state_instances
    state_id = ptr2int(state_ptr)
    if state_id in state_instances:
        state_instances[state_id]._inner_handle()
    else:
        log_error("pypy_state_input() : state {:x} not registered".format(state_id))

@ffi.def_extern(onerror = log_exception)
def pypy_state_blit(state_ptr):
    global state_types, state_instances
    state_id = ptr2int(state_ptr)
    if state_id in state_instances:
        state_instances[state_id]._inner_blit()
    else:
        log_error("pypy_state_blit() : state {:x} not registered".format(state_id))

@ffi.def_extern(onerror = log_exception)
def pypy_state_think(state_ptr):
    global state_types, state_instances
    state_id = ptr2int(state_ptr)
    if state_id in state_instances:
        state_instances[state_id]._inner_think()
    else:
        log_error("pypy_handle_action() : state {:x} not registered".format(state_id))

@ffi.def_extern(onerror = log_exception)
def pypy_game_loaded(game, buffer, bufcap):
    log_info("pypy_game_loaded(game={:x})".format(game))
    lib.prepare_static_mod_data(game)
    pod = None # deserialize it here.
    hooks['game_loaded'](game, pod)

@ffi.def_extern(onerror = log_exception)
def pypy_saving_game(game, buffer, bufcap):
    log_info("pypy_saving_game(game={:x})".format(game))
    pod = saving_game(game)
    # serialize pod into yaml here
    # and write into the buffer.

@ffi.def_extern(onerror = log_exception)
def pypy_game_abandoned(game):
    log_info("pypy_game_abandoned(game={:x})".format(game))
    game_abandoned(game)

@ffi.def_extern(onerror = log_exception)
def pypy_clock_tick(game, step):
    log_info("pypy_clock_tick(game={:x} step={})".format(game, step))
    clock_tick(game, step)

@ffi.def_extern(onerror = log_exception)
def pypy_battle_ended(game):
    log_info("pypy_battle_ended(game={:x})".format(game))
    return battle_ended(game)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_clicked(game, craft, button, kmod):
    log_info("pypy_craft_clicked(game={:x} craft={:x} btn={:x} kmod={:x})".format(game, craft, button, kmod))
    return craft_clicked(game, craft, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_ufo_clicked(game, ufo, button, kmod):
    log_info("pypy_ufo_clicked(game={:x} ufo={:x} btn={:x} kmod={:x})".format(game, base, button, kmod))
    return ufo_clicked(game, ufo, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_base_clicked(game, base, button, kmod):
    log_info("pypy_base_clicked(game={:x} base={:x} btn={:x} kmod={:x})".format(game, base, button, kmod))
    return base_clicked(game, base, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_alienbase_clicked(game, base, button, kmod):
    log_info("pypy_alienbase_clicked(game={:x} base={:x} btn={:x} kmod={:x})".format(game, base, button, kmod))
    return alienbase_clicked(game, base, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_at_target(game, craft, target):
    log_info("pypy_craft_at_target(game={:x} craft={:x} target={:x})".format(game, craft, target))
    return craft_at_target(game, craft, target)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_shot_down(game, craft):
    log_info("pypy_craft_shot_down(game={:x} craft={:x})".format(game, craft))
    return craft_shot_down(game, craft)

@ffi.def_extern(onerror = log_exception)
def pypy_transfer_done(game, transfer):
    log_info("pypy_transfer_done(game={:x} craft={:x})".format(game, transfer))
    return transfer_done(game, transfer)

@ffi.def_extern(onerror = log_exception)
def pypy_research_done(game, research):
    log_info("pypy_research_done(game={:x} craft={:x})".format(game, transfer))
    return research_done(game, transfer)

@ffi.def_extern(onerror = log_exception)
def pypy_manufacture_done(game, manufacture):
    log_info("pypy_manufacture_done(game={:x} craft={:x})".format(game, transfer))
    return manufacture_done(game, transfer)

