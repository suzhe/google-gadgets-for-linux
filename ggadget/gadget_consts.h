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

/** Filenames of required resources in each .gg package. */
const char *const kMainXML         = "main.xml";
const char *const kOptionsXML      = "options.xml";
const char *const kStringsXML      = "strings.xml";
const char *const kGadgetGManifest = "gadget.gmanifest";
const char *const kGManifestExt    = ".gmanifest";

/** Internal config keys */
const char *const kOptionZoom          = "__zoom";
const char *const kOptionDebugMode     = "__debug_mode";

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

} // namespace ggadget

#endif // GGADGET_GADGET_CONSTS_H__
