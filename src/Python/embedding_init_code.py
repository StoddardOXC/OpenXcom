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

def import_modules(drop_old = False):
    global state_types, python_dir
    if drop_old:
        state_types = {}
    for pname in os.listdir(python_dir):
        if  pname != '__pycache__' and (pname.endswith('.py') or os.path.isdir(os.path.join(python_dir, pname))):
            modname = pname
            if pname.endswith('.py'):
                modname = pname[:-3]
            elif os.path.isdir(os.path.join(python_dir, pname)):
                pname += '/'
            log_info("import {} ({})".format(modname, pname))
            try:
                mod = importlib.import_module(modname)
                got_states = set()
                try:
                    mod.__states__
                except Exception as e:
                    log_info("    mod {}: no __states__ : {}".format(modname, e))
                    continue
                for memtuple in inspect.getmembers(mod):
                    memname, member = memtuple
                    if memname in mod.__states__:
                        state_types[memname] = member
                        got_states.add(memname)
                log_info("    imported {} [{}]".format(modname, ', '.join(got_states)))
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
    log_info("PyPy initialized.")
    return 42

@ffi.def_extern(onerror = log_exception)
def pypy_spawn_state(name, parent):
    global state_types, state_instances
    name = ffi.string(name).decode('utf-8')
    try:
        stclass = state_types[name]
    except KeyError:
        log_error("pypy_spawn_state({}): state class not found".format(name))
        return ffi.NULL
    newstate = state_types[name](parent)
    key = int(ffi.cast("uintptr_t", newstate.ptr))
    state_instances[key] = newstate
    log_info("pypy_spawn_state({}): returning {} for {}".format(name, newstate.ptr, newstate))
    return newstate.ptr


@ffi.def_extern(onerror = log_exception)
def pypy_handle_action(action):
    global state_types, state_instances
    """ here we dispatch the action to its target, again """
    log_info("pypy_handle_action(st={:x}, el={:x}, is_mouse={} button={} kmod={:x} key={:x}".format(
        ptr2int(action.state), ptr2int(action.element), action.is_mouse, action.button,
        action.kmod, action.key))

    for ikey, instance in state_instances.items():
        log_info("pypy_handle_action(): state_instance  {:x} -> {!r}".format( ptr2int(ikey), instance))

    state_id = ptr2int(action.state)
    log_info("pypy_handle_action(): target state {:x}".format(state_id))
    if state_id in state_instances:
        state_instances[state_id].handle(action)
    else:
        log_error("pypy_handle_action() : state {:x} not registered".format(state_id))
