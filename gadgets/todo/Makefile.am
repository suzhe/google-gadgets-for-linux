#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

TODO_GADGET_FILES= en/strings.xml \
		  common.js \
		  detail.js \
		  detail.xml \
		  gadget.gmanifest \
		  main.js \
		  main.xml \
		  options.js \
		  options.xml \
		  todo_additem.png \
		  todo_back_down.png \
		  todo_back_over.png \
		  todo_back_up.png \
		  todo_bkgnd_bottom.png \
		  todo_bkgnd_bottomleft.png \
		  todo_bkgnd_bottomright.png \
		  todo_bkgnd_center.png \
		  todo_bkgnd_left.png \
		  todo_bkgnd_right.png \
		  todo_bkgnd_top.png \
		  todo_bkgnd_topleft.png \
		  todo_bkgnd_topright.png \
		  todo_chk_sel_down.png \
		  todo_chk_sel_over.png \
		  todo_chk_sel_up.png \
		  todo_chk_unsel_down.png \
		  todo_chk_unsel_over.png \
		  todo_chk_unsel_up.png \
		  todo_date_selected.png \
		  todo_details_bkgnd.png \
		  todo_divider.png \
		  todo_footer_center.png \
		  todo_footer_left.png \
		  todo_footer_right.png \
		  todo_front_down.png \
		  todo_front_over.png \
		  todo_icon.png \
		  todo_priority_high.png \
		  todo_priority_medium.png \
		  todo_rad_sel_down.png \
		  todo_rad_sel_over.png \
		  todo_rad_sel_up.png \
		  todo_rad_unsel_down.png \
		  todo_rad_unsel_over.png \
		  todo_rad_unsel_up.png

EXTRA_DIST = $(TODO_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
todo_gadget_DATA = todo.gg
todo_gadgetdir=$(GGL_RESOURCE_DIR)

todo.gg: $(TODO_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(TODO_GADGET_FILES))
else
nobase_todo_gadget_DATA = $(TODO_GADGET_FILES)
todo_gadgetdir = $(GGL_RESOURCE_DIR)/todo
endif
