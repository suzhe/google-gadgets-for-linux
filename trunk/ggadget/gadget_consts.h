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

/** Character to separate directories */
const char kDirSeparator = '/';
const char kDirSeparatorStr[] = "/";

/** Character to separate search paths, eg. used in PATH environment var */
const char kSearchPathSeparator = ':';
const char kSearchPathSeparatorStr[] = ":";

/** Standard suffix of gadget file. */
const char kGadgetFileSuffix[] = ".gg";

/** Filenames of required resources in each .gg package. */
const char kMainXML[]         = "main.xml";
const char kOptionsXML[]      = "options.xml";
const char kStringsXML[]      = "strings.xml";
const char kGadgetGManifest[] = "gadget.gmanifest";
const char kGManifestExt[]    = ".gmanifest";
const char kXMLExt[]          = ".xml";

/** Information XPath identifiers in gadget.gmanifest file. */
const char kManifestMinVersion[]  = "@minimumGoogleDesktopVersion";
const char kManifestId[]          = "about/id";
const char kManifestName[]        = "about/name";
const char kManifestDescription[] = "about/description";
const char kManifestVersion[]     = "about/version";
const char kManifestAuthor[]      = "about/author";
const char kManifestAuthorEmail[] = "about/authorEmail";
const char kManifestAuthorWebsite[]
                                  = "about/authorWebsite";
const char kManifestCopyright[]   = "about/copyright";
const char kManifestAboutText[]   = "about/aboutText";
const char kManifestSmallIcon[]   = "about/smallIcon";
const char kManifestIcon[]        = "about/icon";
const char kManifestDisplayAPI[]  = "about/displayAPI";
const char kManifestDisplayAPIUseNotifier[]
                                  = "about/displayAPI@useNotifier";
const char kManifestEventAPI[]    = "about/eventAPI";
const char kManifestQueryAPI[]    = "about/queryAPI";
const char kManifestQueryAPIAllowModifyIndex[]
                                  = "about/queryAPI@allowModifyIndex";

/**
 * To enumerate all fonts to be installed, you must enumerate the manifest
 * map and use SimpleMatchXPath(key, kManifestInstallFontSrc) to test if the
 * value is a font src attribute.
 */
const char kManifestInstallFontSrc[] = "install/font@src";

/**
 * To enumerate all objects to be installed, you must enumerate the manifest
 * map and use SimpleMatchXPath(key, kManifestInstallObject) and then
 * for each of the config items found, access their attributes with keys by
 * postpending "@name", "@clsid", "@src" to the keys.
 */
const char kManifestInstallObject[] = "install/object";
/**
 * Simple way to enumerate all objects to be installed, ignoring "name" and
 * "clsid" attributes.
 */
const char kManifestInstallObjectSrc[] = "install/object@src";

/** The tag name of the root element in string table files. */
const char kStringsTag[] = "strings";
/** The tag name of the root element in view file. */
const char kViewTag[] = "view";
/** The tag name of the root element in gadget.gmanifest files. */
const char kGadgetTag[] = "gadget";
/** The tag name of script elements */
const char kScriptTag[] = "script";
/** The name of the 'src' attribute of script elements. */
const char kSrcAttr[] = "src";
/** The name of the 'name' attribute of elements. */
const char kNameAttr[] = "name";
/** The property name for elements to contain its text contents. */
const char kInnerTextProperty[] = "innerText";
/** The tag name of the contentarea element. */
const char kContentAreaTag[] = "contentarea";

/** Prefix for user profile files. */
const char kProfilePrefix[] = "profile://";

/** Prefix for global file resources. */
const char kGlobalResourcePrefix[] = "resource://";

/** Default directory to store profiles. */
const char kDefaultProfileDirectory[] = ".google/gadgets";

const char kScrollDefaultBackgroundH[] = "resource://scroll_background_h.png";
const char kScrollDefaultGrippyH[] = "resource://scrollbar_grippy_h.png";
const char kScrollDefaultThumbH[] = "resource://scrollbar_u_h.png";
const char kScrollDefaultThumbDownH[] = "resource://scrollbar_d_h.png";
const char kScrollDefaultThumbOverH[] = "resource://scrollbar_o_h.png";
const char kScrollDefaultLeft[] = "resource://scrollleft_u.png";
const char kScrollDefaultLeftDown[] = "resource://scrollleft_d.png";
const char kScrollDefaultLeftOver[] = "resource://scrollleft_o.png";
const char kScrollDefaultRight[] = "resource://scrollright_u.png";
const char kScrollDefaultRightDown[] = "resource://scrollright_d.png";
const char kScrollDefaultRightOver[] = "resource://scrollright_o.png";

const char kScrollDefaultBackgroundV[] = "resource://scroll_background.png";
const char kScrollDefaultGrippyV[] = "resource://scrollbar_grippy.png";
const char kScrollDefaultThumbV[] = "resource://scrollbar_u.png";
const char kScrollDefaultThumbDownV[] = "resource://scrollbar_d.png";
const char kScrollDefaultThumbOverV[] = "resource://scrollbar_o.png";
const char kScrollDefaultUp[] = "resource://scrollup_u.png";
const char kScrollDefaultUpDown[] = "resource://scrollup_d.png";
const char kScrollDefaultUpOver[] = "resource://scrollup_o.png";
const char kScrollDefaultDown[] = "resource://scrolldown_u.png";
const char kScrollDefaultDownDown[] = "resource://scrolldown_d.png";
const char kScrollDefaultDownOver[] = "resource://scrolldown_o.png";

const char kComboArrow[] = "resource://combo_arrow_up.png";
const char kComboArrowDown[] = "resource://combo_arrow_down.png";
const char kComboArrowOver[] = "resource://combo_arrow_over.png";

const char kContentItemUnpinned[] = "resource://unpinned.png";
const char kContentItemUnpinnedOver[] = "resource://unpinned_over.png";
const char kContentItemPinned[] = "resource://pinned.png";

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

// The following are used by the view decorator.
const char kVDButtonBackground[] = "resource://vd_button_background.png";
const char kVDButtonCloseNormal[] = "resource://vd_close_normal.png";
const char kVDButtonCloseOver[] = "resource://vd_close_over.png";
const char kVDButtonCloseDown[] = "resource://vd_close_down.png";
const char kVDButtonMenuNormal[] = "resource://vd_menu_normal.png";
const char kVDButtonMenuOver[] = "resource://vd_menu_over.png";
const char kVDButtonMenuDown[] = "resource://vd_menu_down.png";
const char kVDButtonBackNormal[] = "resource://vd_back_normal.png";
const char kVDButtonBackOver[] = "resource://vd_back_over.png";
const char kVDButtonBackDown[] = "resource://vd_back_down.png";
const char kVDButtonForwardNormal[] = "resource://vd_forward_normal.png";
const char kVDButtonForwardOver[] = "resource://vd_forward_over.png";
const char kVDButtonForwardDown[] = "resource://vd_forward_down.png";
const char kVDButtonExpandNormal[] = "resource://vd_expand_normal.png";
const char kVDButtonExpandOver[] = "resource://vd_expand_over.png";
const char kVDButtonExpandDown[] = "resource://vd_expand_down.png";
const char kVDButtonUnexpandNormal[] = "resource://vd_unexpand_normal.png";
const char kVDButtonUnexpandOver[] = "resource://vd_unexpand_over.png";
const char kVDButtonUnexpandDown[] = "resource://vd_unexpand_down.png";

const char kVDBorderH[] = "resource://vd_border_h.png";
const char kVDBorderV[] = "resource://vd_border_v.png";
const char kVDMainBackground[] = "resource://vd_main_background.png";

const char kSideBarIcon[] = "resource://sidebar_google.png";
const char kSBButtonAddDown[] = "resource://sidebar_add_down.png";
const char kSBButtonAddUp[] = "resource://sidebar_add_up.png";
const char kSBButtonAddOver[] = "resource://sidebar_add_over.png";
const char kSBButtonConfigDown[] = "resource://sidebar_config_down.png";
const char kSBButtonConfigUp[] = "resource://sidebar_config_up.png";
const char kSBButtonConfigOver[] = "resource://sidebar_config_over.png";
const char kSBButtonCloseDown[] = "resource://sidebar_close_down.png";
const char kSBButtonCloseUp[] = "resource://sidebar_close_up.png";
const char kSBButtonCloseOver[] = "resource://sidebar_close_over.png";

const char kCommonJS[] = "resource://common.js";
const char kTextDetailsView[] = "resource://text_details_view.xml";
const char kHTMLDetailsView[] = "resource://html_details_view.xml";

const char kFtpUrlPrefix[] = "ftp://";
const char kHttpUrlPrefix[] = "http://";
const char kHttpsUrlPrefix[] = "https://";
const char kFeedUrlPrefix[] = "feed://";
const char kFileUrlPrefix[] = "file://";

/** The fallback encoding used to parse text files. */
const char kEncodingFallback[] = "ISO8859-1";

} // namespace ggadget

#endif // GGADGET_GADGET_CONSTS_H__
