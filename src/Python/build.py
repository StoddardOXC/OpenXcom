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

import cffi
ffibuilder = cffi.FFI()

def filter_a_header(fname):
  with open(fname) as f:
      # read the header and pass it to embedding_api(), manually
      # removing the '#' directives and the CFFI_DLLEXPORT
      # and also extern "C" -s.
      data = ''.join([line for line in f if (not line.startswith('#')
                      and (not line.startswith('class'))
                      and (not 'extern "C"' in line))])
      data = data.replace('CFFI_DLLEXPORT', '')
  return data

ffibuilder.embedding_api(filter_a_header('module.h'))

ffibuilder.cdef(filter_a_header('adapter.h'))

#ffibuilder.cdef("""
#extern "Python" void pypy_action_handler(action_t);
#""")

ffibuilder.set_source("pypycom", r'''
    #include "module.h"
    #include "adapter.h"
''')

ffibuilder.embedding_init_code(open("embedding_init_code.py").read())

ffibuilder.emit_c_code("module.c")
