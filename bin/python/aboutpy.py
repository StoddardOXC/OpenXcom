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

#
# about-pypy state/popup
#

# this lists state objects the module exports
__states__ = ['AboutPyPy', 'BaseFacilityList']

# this gets base classes and stuff
from api import *

import sys

class AboutPyPy(State):
    def __init__(self, parent):
        st_w = 300
        btn_w = 60
        super(AboutPyPy, self).__init__(parent,
            w = st_w, h = 180, x = 10, y = 10,
            popmode = POPUP_BOTH, ui_category = "geoscape", bg = "BACK01.SCR")

        version = "Python " + sys.version.replace(' (', ' \n(' )
        self.add_text(280, 140, 20, 20, "text", "saveMenus", ALIGN_CENTER, ALIGN_MIDDLE, False, False, version)
        btn = TextButton(self, btn_w, 20, st_w/2 - btn_w/2, 160, "button", "saveMenus", self.tr("STR_OK"));
        btn.onMouseClick(self.btnOk, 1)
        btn.onKeyboardPress(self.btnOk) # no keysym = any key
        self.add(btn)

    def btnOk(self, *args):
        self.pop()

class BaseFacilityList(State):
    def __init__(self, parent):
        st_w = 300
        btn_w = 60
        super(BaseFacilityList, self).__init__(parent,
            w = st_w, h = 180, x = 10, y = 10,
            popmode = POPUP_BOTH, ui_category = "geoscape", bg = "BACK01.SCR")

        version = "Python " + sys.version.replace(' (', ' \n(' )
        tl = TextList(self, 280, 140, 20, 20, "text", "saveMenus")
        tl.set_columns(80, 80, 80)
        #tl.set_arrow_column(182, ARROW_VERTICAL);

        btn = TextButton(self, btn_w, 20, st_w/2 - btn_w/2, 160, "button", "saveMenus", self.tr("STR_OK"));
        btn.onMouseClick(self.btnOk, 1)
        btn.onKeyboardPress(self.btnOk) # no keysym = any key
        self.add(tl)
        self.add(btn)

        bases = Game.get_bases(self)
        if bases is None or len(bases) == 0:
            log_info("no bases")
            return

        for base in bases:
            log_info("{} : {}".format(base['name'], '\n  '.join(f['type'] for f in base['facilities'])))
            tl.add_row(base['name'], "shit", "crap")
            for fac in (f['type'] for f in base['facilities']):
                tl.add_row("", fac, str(fac["maintenance"]))

    def btnOk(self, *args):
        self.pop()
