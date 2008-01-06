/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_GADGET_CONSTS_H__
#define GGADGET_GADGET_CONSTS_H__

#include <map>
#include <string>

namespace ggadget {

/** Path separator used in Linux platform */
const char kPathSeparator = '/';
const char *const kPathSeparatorStr = "/";

/** Filenames of required resources in each .gg package. */
const char *const kMainXML         = "main.xml";
const char *const kOptionsXML      = "options.xml";
const char *const kStringsXML      = "strings.xml";
const char *const kGadgetGManifest = "gadget.gmanifest";
const char *const kGManifestExt    = ".gmanifest";
const char *const kXMLExt          = ".xml";

/** Information XPath identifiers in gadget.gmanifest file. */
const char *const kManifestMinVersion  = "@minimumGoogleDesktopVersion";
const char *const kManifestId          = "about/id";
const char *const kManifestName        = "about/name";
const char *const kManifestDescription = "about/description";
const char *const kManifestVersion     = "about/version";
const char *const kManifestAuthor      = "about/author";
const char *const kManifestAuthorEmail = "about/authorEmail";
const char *const kManifestAuthorWebsite
                                       = "about/authorWebsite";
const char *const kManifestCopyright   = "about/copyright";
const char *const kManifestAboutText   = "about/aboutText";
const char *const kManifestSmallIcon   = "about/smallIcon";
const char *const kManifestIcon        = "about/icon";
const char *const kManifestDisplayAPI  = "about/displayAPI";
const char *const kManifestDisplayAPIUseNotifier
                                       = "about/displayAPI@useNotifier";
const char *const kManifestEventAPI    = "about/eventAPI";
const char *const kManifestQueryAPI    = "about/queryAPI";
const char *const kManifestQueryAPIAllowModifyIndex
                                       = "about/queryAPI@allowModifyIndex";

/**
 * To enumerate all fonts to be installed, you must try the following keys:
 *   - <code>"install/font@src"    (kManifestInstallFont + "@src")</code>
 *   - <code>"install/font[1]@src" (kManifestInstallFont + "[1]@src")</code>
 *   - <code>"install/font[2]@src" (kManifestInstallFont + "[2]@src")</code>
 *   - ... until not found.
 */
const char *const kManifestInstallFont = "install/font";

/**
 * To enumerate all objects to be installed, you must try the following keys:
 *   - <code>"install/object"    (kManifestInstallObject)</code>
 *   - <code>"install/object[1]" (kManifestInstallObject + "[1]")</code>
 *   - <code>"install/object[2]" (kManifestInstallObject + "[2]")</code>
 *   - ... until not found.
 * For each of the config items found, access their attributes with keys by
 * postpending "@name", "@clsid", "@src" to the above keys.
 */
const char *const kManifestInstallObject = "install/object";

/** The tag name of the root element in string table files. */
const char *const kStringsTag = "strings";
/** The tag name of the root element in view file. */
const char *const kViewTag = "view";
/** The tag name of the root element in gadget.gmanifest files. */
const char *const kGadgetTag = "gadget";
/** The tag name of script elements */
const char *const kScriptTag = "script";
/** The name of the 'src' attribute of script elements. */
const char *const kSrcAttr = "src";
/** The name of the 'name' attribute of elements. */
const char *const kNameAttr = "name";
/** The property name for elements to contain its text contents. */
const char *const kInnerTextProperty = "innerText";
/** The tag name of the contentarea element. */
const char *const kContentAreaTag = "contentarea";

/** Prefix for global file resources. */
const char kGlobalResourcePrefix[] = "resource://";

const char kScrollDefaultBackground[] = "resource://scroll_background.png";
const char kScrollDefaultThumb[] = "resource://scrollbar_u.png";
const char kScrollDefaultThumbDown[] = "resource://scrollbar_d.png";
const char kScrollDefaultThumbOver[] = "resource://scrollbar_o.png";
const char kScrollDefaultLeft[] = "resource://scrollleft_u.png";
const char kScrollDefaultLeftDown[] = "resource://scrollleft_d.png";
const char kScrollDefaultLeftOver[] = "resource://scrollleft_o.png";
const char kScrollDefaultRight[] = "resource://scrollright_u.png";
const char kScrollDefaultRightDown[] = "resource://scrollright_d.png";
const char kScrollDefaultRightOver[] = "resource://scrollright_o.png";

const char kContentItemPinned[] = "resource://pinned.png";
const char kContentItemUnpinned[] = "resource://unpinned.png";
const char kContentItemPinnedOver[] = "resource://pinned_over.png";

// The following kButtonXXX and kCheckBoxXXX images are not for element
// defaults, but for convenience of native gadgets.
const char kButtonImage[] = "resource://button_up.png";
const char kButtonDownImage[] = "resource://button_down.png";
const char kButtonOverImage[] = "resource://button_over.png";
const char kCheckBoxImage[] = "resource://checkbox_up.png";
const char kCheckBoxDownImage[] = "resource://checkbox_down.png";
const char kCheckBoxOverImage[] = "resource://checkbox_over.png";
const char kCheckBoxCheckedImage[] = "resource://checkbox_checked_up.png";
const char kCheckBoxCheckedDownImage[] = "resource://checkbox_checked_down.png";
const char kCheckBoxCheckedOverImage[] = "resource://checkbox_checked_over.png";

const char kCommonJS[] = "resource://common.js";
const char kDetailsView[] = "resource://details_view.xml";

} // namespace ggadget

#endif // GGADGET_GADGET_CONSTS_H__
