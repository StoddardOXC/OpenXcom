#
# Copyright 2018 Stoddard, https://github.com/StoddardOXC.
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

import sys, os, os.path, importlib, inspect, traceback, pprint, io
from ruamel.yaml import YAML
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

GAME = None
state_types = {}
state_instances = {}



# load_options hook is called right after importing stuff is completed.
# mods_loaded hook is called after that, i think..
# damn, need to sort out and document all that.
#

# default hooks:
import ruamel.yaml
class _DefaultHookNamespace(object):
    @staticmethod
    def mods_loaded():
        print("defhooks: mods_loaded")
    @staticmethod
    def game_abandoned():
        print("defhooks: game_abandoned")
    @staticmethod
    def game_loaded(pod): # pod = Plain Old Data: something deserealized from yaml.
        print("defhooks: game_loaded {!r}".format(pod))
    @staticmethod
    def saving_game():
        print("defhooks: saving_game")
    @staticmethod
    def game_saved():
        print("defhooks: game_saved")
    @staticmethod
    def set_options(pod): # pod = options desearialized from yaml
        print("defhooks: set_options {!r}".format(pod))
    @staticmethod
    def get_options():
        print("defhooks: get_options")

    @staticmethod
    def time_5sec():
        print("defhooks: ")
    @staticmethod
    def time_10min():
        print("defhooks: ")
    @staticmethod
    def time_30min():
        print("defhooks: ")
    @staticmethod
    def time_1hour():
        print("defhooks: ")
    @staticmethod
    def time_1day():
        print("defhooks: ")
    @staticmethod
    def time_1mon():
        print("defhooks: ")

    @staticmethod
    def mainmenu_keydown(k, m):
        print("defhooks: mainmenu_keydown")
        return 1
    @staticmethod
    def geoscape_keydown(k, m):
        print("defhooks: geoscape_keydown")
        return 1
    @staticmethod
    def battle_ended():
        print("defhooks: battle_ended")
        return 1
    @staticmethod
    def craft_shot_down(craft):
        print("defhooks: craft_shot_down")
        return 1
    @staticmethod
    def craft_at_target(target):
        print("defhooks: ")
        return 1
    @staticmethod
    def alienbase_clicked(base, button, kmod):
        print("defhooks: ")
        return 1
    @staticmethod
    def base_clicked(base, button, kmod):
        print("defhooks: ")
        return 1
    @staticmethod
    def ufo_clicked(ufo, button, kmod):
        print("defhooks: ufo_clicked")
        return 1
    @staticmethod
    def craft_clicked(craft, button, kmod):
        print("defhooks: craft_clicked")
        return 1

    @staticmethod
    def transfer_done(transfer):
        print("defhooks: transfer_done")
        return 1
    @staticmethod
    def research_done(research):
        print("defhooks: research_done")
        return 1
    @staticmethod
    def manufacture_done(manufacture):
        print("defhooks: manufacture_done")
        return 1

class Hooks(object):
    _hook_list = []
    @classmethod
    def _add_hook(klass, name, callable):
        setattr(klass, name, callable)
        klass._hook_list.append(name)

    @classmethod
    def _reset(klass):
        for name, member in inspect.getmembers(_DefaultHookNamespace):
            if not name.startswith('_'):
                klass._add_hook(name, member)

Hooks._reset() # this can be done even more hairer with inheritance, but next time

def import_modules(python_dir, drop_old = False):
    global state_types
    if drop_old:
        state_types = {}
        Hooks._reset()
    got_hooks = set()
    got_states = set()
    for pname in os.listdir(python_dir):
        if  pname != '__pycache__' and (pname.endswith('.py') or os.path.isdir(os.path.join(python_dir, pname))):
            modname = pname
            if pname.endswith('.py'):
                modname = pname[:-3]
            elif os.path.isdir(os.path.join(python_dir, pname)):
                pname += '/'
            log_info("import {} ({})".format(modname, os.path.join(python_dir, pname)))
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
                    got_states.add(memname)
                elif memname in mod_hooks:
                    Hooks._add_hook(memname, member)
                    got_stuff.add(memname)
                    got_hooks.add(memname)
            log_info("    imported {} [{}]".format(modname, ', '.join(got_stuff)))
    return got_states, got_hooks

@ffi.def_extern(onerror = log_exception)
def pypy_initialize(game):
    global GAME, state_types
    state_types = {} # forget state types left from the previus modset
    Hooks._reset()   # forget hooks left from the previus modset

    log_info("Python " + sys.version.replace('\n', ' '))
    GAME = game
    lib.set_game_ptr(game)

#
# Mod loading:
#
# A mod has to provide a single pythonpath.
#
# Only last mod loaded is currently supported. Hooks are being reset between mod scans.
#
# All enabled mods' pythondirs are scanned and imported (provides code execution at module level)
#
# This is subject to change at a later date to both:  TODO
#   - enable different mods to claim separate subsets of hooks
#   - chain hook calls in (some) order of mods having been loaded subject to hooks' retval
#
#
# Since OXCE's loadFile function has zero idea about the mod root path and I'd rather not
# change that, or save pythonDir in the Mod object,  first the Mod::loadAll() calls this
# to set the currently loading mods' root
#
_current_modpath = None
@ffi.def_extern(onerror = log_exception)
def pypy_set_modpath(modpath):
    global _current_modpath
    _current_modpath = ffi.string(modpath).decode('utf-8')
#
# Then if mod defines a pythonDir, this gets called, with python_dir being relative to mod root.
# Can in fact define several, but currently only the last one will be useful
#
@ffi.def_extern(onerror = log_exception)
def pypy_hook_up(python_dir):
    Hooks._reset()  # reset what previous mods did
    if python_dir == ffi.NULL: # okay, just reset hooks. equal to being disabled.
        log_info("PyPy: pypy_hook_up(): reset.")
        return

    fullpydir = os.path.realpath(os.path.join(_current_modpath, ffi.string(python_dir).decode("utf-8")))
    if os.path.isdir(fullpydir):
        log_info("PyPy: pypy_hook_up(): {} exists, importing".format(fullpydir))
        sys.path.append(fullpydir)
        states_got, hooks_got = import_modules(fullpydir)
    else:
        log_info("PyPy: '{}' does not exist, ignoring".format(fullpydir))
        return

    log_info("PyPy sys.path:")
    for p in sys.path:
        log_info("    ", p)

    log_info("PyPy states defined:")
    for stname in states_got:
        log_info("    ", stname)

    log_info("PyPy hooks set:")
    for hkname in hooks_got:
        log_info("    ", hkname)

    Hooks.set_options(options_pod)

options_pod = None # dict or OrderedDict of pypy-specific options.
# keeps'em alive between initial option load and the actual py module imports (see above)
options_bytes = b"" # pin for the data returned to oxce
# called before saving options to disk.
# parameter: non-oxce options as yaml.
# expected rv -yaml in bytes as in what to actually save.
@ffi.def_extern(onerror = log_exception)
def pypy_get_options():
    global options_pod, options_bytes
    log_info("pypy_get_options")
    options_pod = Hooks.get_options()
    if type(options_pod) is not dict:
        log_error("Hooks.get_options() returned not dict: {!r}".format(options_pod))
        return ffi.NULL
    yaml = YAML()
    yaml.default_flow_style = False
    options_bytes = yaml.dump({ "pypy": options_pod })
    log_info("pypy_get_options returnning\n{}".format(options_bytes.decode("utf-8")))
    #print("pypy_get_options returnning\n{}".format(options_bytes.decode("utf-8")))
    return options_bytes

# called after loading options from disk
# or resetting them . yaml_opts = all options as an utf8z
# hook gets called with the relevant const dict/OrderedDict, tag is pypy in options.cfg.
@ffi.def_extern(onerror = log_exception)
def pypy_set_options(yaml_opts):
    global options_pod
    options_pod = {}
    if yaml_opts == ffi.NULL:
        # well, log this somehow TODO
        return
    yaml_opts = ffi.string(yaml_opts)
    log_info("pypy_load_options(): got\n{}".format(yaml_opts.decode("utf-8")))
    yaml= YAML()
    try:
        all_options = yaml.load(yaml_opts)
    except Exception as e:
        estr = "pypy_load_options(): got yaml parse exception: {}".format(e)
        print(estr) # this can happen before logging is functional TODO FIXME
        log_error(estr)
    else:
        try:
            options_pod = all_options['pypy']
        except Exception as e:
            log_info("no pypy options loaded {!r} ".format(e))
            print("no pypy options loaded {!r} ".format(e))
    Hooks.set_options(options_pod)

#
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
# The current scheme is that an event comes in via state_t's overrides for
# State::handle(), State::blit() and State::think(), which call pypy_ equivalents,
# which call python methods of a python State this state_t is associated with.
#
# For pypy_ functions to know what object's methods to call this has to keep
# some sort of mapping between state_t * ptr and the python object to call upon.
#
# Currently the python object is created in pypy_spawn_state and that's how we get
# a reference to it. A method to pass py object ref from C code might be there
# but I'm currently too lazy to discover it.
#
# okay. so the api.py State constructor calls a ffi.new_handle(self)
# passes that to lib.new_state() which stores it.
#
# state_t::handle/blit/think do call pypy_call_method(sthandle, methodname)
#
# pypy_call_method(sthandle, methodname) does
#  getattr(ffi.from_hanlde(sthandle), methodname)()
#
# given that handle/blit/think do not have any arguments this should work.
#
#
# So the overall philosophy is that events are supplied to the hooks defined
# in the imported python code. The state_t is just a fat hook inserted into
# the game's state stack.
#
#
#
# This is what pypy_spawn_state() is for. It also does GAME->push_state() via the lib.
#
#
#
#
#
# A game frame is processed as follows:
#
# - all input events are converted to Action-s and fed to the topmost State only.
# - topmost State's think() method is called.
# - all States' blit() method is called.
#
#
#@ffi.def_extern(onerror = log_exception)
#def pypy_spawn_state(name):
    #global state_types, state_instances
    #name = ffi.string(name).decode('utf-8')
    #try:
        #stclass = state_types[name]
    #except KeyError:
        #log_error("pypy_spawn_state({}): state class not found".format(name))
        ## TODO: return the error as a state so as not to crash.
        #return lib.state_not_found(name.encode("utf-8"));
    #newstate = state_types[name]()
    #state_id = ptr2int(newstate.ptr)
    #state_instances[state_id] = newstate
    ##log_info("pypy_spawn_state({}): returning {} for {}".format(name, newstate.ptr, newstate))
    #lib.push_state(newstate.ptr)
    #return newstate.ptr

## lib.pop_state() calls this, then calls GAME->popState() where delete (*state_ptr) is done.
## this just removes the state_id and ref to its instance from the map.
#@ffi.def_extern(onerror = log_exception)
#def pypy_forget_state(state_ptr):
    #state_id = ptr2int(state_ptr)
    #del state_instances[state_id]
    ##log_info("pypy_forget_state({}) done.".format(state_ptr))

@ffi.def_extern(onerror = log_exception)
def pypy_call_method(ffihandle, methodname):
    meth = ffi.string(methodname).decode('utf-8')
    pyobj = ffi.from_handle(ffihandle)
    #print("pypy_call_method({!r}, {}".format(ffihandle, meth))
    getattr(pyobj, meth)()

#@ffi.def_extern(onerror = log_exception)
#def pypy_state_input(state_ptr):
    #global state_types, state_instances
    #state_id = ptr2int(state_ptr)
    #if state_id in state_instances:
        #state_instances[state_id]._inner_input()
    #else:
        #log_error("pypy_state_input() : state {:x} not registered".format(state_id))

#@ffi.def_extern(onerror = log_exception)
#def pypy_state_blit(state_ptr):
    #global state_types, state_instances
    #state_id = ptr2int(state_ptr)
    #if state_id in state_instances:
        #state_instances[state_id]._inner_blit()
    #else:
        #log_error("pypy_state_blit() : state {:x} not registered".format(state_id))

#@ffi.def_extern(onerror = log_exception)
#def pypy_state_think(state_ptr):
    #global state_types, state_instances
    #state_id = ptr2int(state_ptr)
    #if state_id in state_instances:
        #state_instances[state_id]._inner_think()
    #else:
        #log_error("pypy_state_think() : state {:x} not registered".format(state_id))

@ffi.def_extern(onerror = log_exception)
def pypy_mods_loaded():
    log_info("pypy_mods_loaded(): slurping constants")
    lib.prepare_static_mod_data()
    Hooks.mods_loaded()

@ffi.def_extern(onerror = log_exception)
def pypy_lang_changed():
    log_info("pypy_lang_changed(): dropping translations")
    lib.drop_translations()

@ffi.def_extern(onerror = log_exception)
def pypy_game_loaded(buffer, bufsize):
    log_info("pypy_game_loaded(bufsize={:d})".format(bufsize))
    yaml= YAML()
    #yaml.default_flow_style = False
    yaml_str = bytes(ffi.buffer(buffer, bufsize)).decode('utf8')
    log_info("pypy_game_loaded(): got {!r}".format(yaml_str))
    try:
        pod = yaml.load(yaml_str)#, Loader=ruamel.yaml.Loader)
    except Exception as e:
        log_error("pypy_game_loaded(): failure deserializing - ignoring: {!r}".format(yaml_str))
        pod = None

    Hooks.game_loaded(pod)

pypydata_pin = None
@ffi.def_extern(onerror = log_exception)
def pypy_saving_game(buf):
    global pypydata_pin
    buf.ptr = ffi.NULL
    buf.size = 0
    pod = Hooks.saving_game()
    if pod is None:
        log_info("pypy_saving_game(): got None, saving nothing")
        return
    yaml = YAML()
    yaml.default_flow_style = False
    ystream = io.StringIO()
    yaml.dump(pod, ystream)
    yaml_str = ystream.getvalue()
    # must pin the bytes object until the game is done saving.
    pypydata_pin = yaml_str.encode('utf-8')
    buf.ptr = ffi.from_buffer(pypydata_pin)
    buf.size = len(pypydata_pin)

@ffi.def_extern(onerror = log_exception)
def pypy_game_saved():
    global pypydata_pin
    log_info("pypy_game_saved(): released pin")
    pypydata_pin = None

@ffi.def_extern(onerror = log_exception)
def pypy_game_abandoned():
    log_info("pypy_game_abandoned()")
    Hooks.game_abandoned()

@ffi.def_extern(onerror = log_exception)
def pypy_time_5sec():
    log_debug("pypy_time_5sec()")
    Hooks.time_5sec()

@ffi.def_extern(onerror = log_exception)
def pypy_time_10min():
    log_info("pypy_time_10min()")
    Hooks.time_10min()

@ffi.def_extern(onerror = log_exception)
def pypy_time_30min():
    log_info("pypy_time_30min()")
    Hooks.time_30min()

@ffi.def_extern(onerror = log_exception)
def pypy_time_1hour():
    log_info("pypy_time_1hour()")
    Hooks.time_1hour()

@ffi.def_extern(onerror = log_exception)
def pypy_time_1day():
    log_info("pypy_time_1day()")
    Hooks.time_1day()

@ffi.def_extern(onerror = log_exception)
def pypy_time_1mon():
    log_info("pypy_time_1mon()")
    Hooks.time_1mon()

@ffi.def_extern(onerror = log_exception)
def pypy_battle_ended():
    log_info("pypy_battle_ended()")
    return Hooks.battle_ended()

@ffi.def_extern(onerror = log_exception)
def pypy_mainmenu_keydown(sym, mods):
    log_debug("pypy_mainmenu_keydown() : {:x}|{:x}".format(sym, mods))
    return Hooks.mainmenu_keydown(sym, mods)

@ffi.def_extern(onerror = log_exception)
def pypy_geoscape_keydown(sym, mods):
    log_debug("pypy_geoscape_keydown() : {:x}|{:x}".format(sym, mods))
    return Hooks.geoscape_keydown(sym, mods)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_clicked(craft, button, kmod):
    log_info("pypy_craft_clicked(craft={:x} btn={:x} kmod={:x})".format(craft, button, kmod))
    return Hooks.craft_clicked(craft, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_ufo_clicked(ufo, button, kmod):
    log_info("pypy_ufo_clicked(ufo={:x} btn={:x} kmod={:x})".format(base, button, kmod))
    return Hooks.ufo_clicked(ufo, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_base_clicked(base, button, kmod):
    log_info("pypy_base_clicked(base={:x} btn={:x} kmod={:x})".format(base, button, kmod))
    return Hooks.base_clicked(base, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_alienbase_clicked(base, button, kmod):
    log_info("pypy_alienbase_clicked(base={:x} btn={:x} kmod={:x})".format(base, button, kmod))
    return Hooks.alienbase_clicked(base, button, kmod)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_at_target(craft, target):
    log_info("pypy_craft_at_target(craft={:x} target={:x})".format(craft, target))
    return Hooks.craft_at_target(craft, target)

@ffi.def_extern(onerror = log_exception)
def pypy_craft_shot_down(craft):
    log_info("pypy_craft_shot_down(craft={:x})".format(craft))
    return Hooks.craft_shot_down(craft)

@ffi.def_extern(onerror = log_exception)
def pypy_transfer_done(transfer):
    log_info("pypy_transfer_done(craft={:x})".format(transfer))
    return Hooks.transfer_done(transfer)

@ffi.def_extern(onerror = log_exception)
def pypy_research_done(research):
    log_info("pypy_research_done(craft={:x})".format(research))
    return Hooks.research_done(research)

@ffi.def_extern(onerror = log_exception)
def pypy_manufacture_done(manufacture):
    log_info("pypy_manufacture_done(craft={:x})".format(manufacture))
    return Hooks.manufacture_done(manufacture)

