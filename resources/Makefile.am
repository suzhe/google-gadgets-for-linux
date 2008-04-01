#
# Copyright 2007 Google Inc.
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
#

RESOURCE_FILES= button_down.png \
		button_over.png \
		button_up.png \
		checkbox_checked_down.png \
		checkbox_checked_over.png \
		checkbox_checked_up.png \
		checkbox_down.png \
		checkbox_over.png \
		checkbox_up.png \
		combo_arrow_up.png \
		combo_arrow_down.png \
		combo_arrow_over.png \
		common.js \
		html_details_view.xml \
		messages-catalog.xml \
		pinned.png \
		scroll_background_h.png \
		scrollbar_d_h.png \
		scrollbar_grippy_h.png \
		scrollbar_o_h.png \
		scrollbar_u_h.png \
		scrollleft_d.png \
		scrollleft_o.png \
		scrollleft_u.png \
		scrollright_d.png \
		scrollright_o.png \
		scrollright_u.png \
		scroll_background.png \
		scrollbar_d.png \
		scrollbar_grippy.png \
		scrollbar_o.png \
		scrollbar_u.png \
		scrollup_d.png \
		scrollup_o.png \
		scrollup_u.png \
		scrolldown_d.png \
		scrolldown_o.png \
		scrolldown_u.png \
		text_details_view.xml \
		unpinned.png \
		unpinned_over.png \
		en/strings.xml \
		zh-CN/strings.xml

EXTRA_DIST=$(RESOURCE_FILES)

if GGL_HAVE_ZIP
resource_DATA = resources.gg
resourcedir=$(GGL_RESOURCE_DIR)

resources.gg: $(RESOURCE_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(RESOURCE_FILES))
else
nobase_resource_DATA = $(RESOURCE_FILES)
resourcedir=$(GGL_RESOURCE_DIR)/resources
endif
