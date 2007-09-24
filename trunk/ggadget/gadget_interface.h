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

#ifndef GGADGET_GADGET_INTERFACE_H__
#define GGADGET_GADGET_INTERFACE_H__

namespace ggadget {

class FileManagerInterface;
class ViewInterface;

/**
 * Interface for representing a Gadget in the Gadget API.
 */
class GadgetInterface {
 public:
  virtual ~GadgetInterface() { }

  /**
   * @return the ViewInterface object representing the main view of the gadget.
   */
  virtual ViewInterface *GetMainView() = 0;

  /**
   * @return the ViewInterface object representing the options
   * view of the gadget. Returns NULL if this view is not supported.
   */
  virtual ViewInterface *GetOptionsView() = 0;

  /**
   * @return the FileManagerInterface object used in this gadget.
   */
  virtual FileManagerInterface *GetFileManager() = 0;

  /**
   * Get a value configured in the gadget manifest file.
   * @param key the value key like a simple XPath expression. See
   *     gadget_consts.h for available keys, and @c ParseXMLIntoXPathMap() in
   *     xml_utils.h for details of the XPath expression.
   * @return the configured value. @c NULL if not found.
   */
  virtual const char *GetManifestInfo(const char *key) = 0;

  /**
   * When the gadget is running in debug mode, displays the string Message in
   * the debug console as an error.
   */
  virtual void DebugError(const char *message) = 0;

  /**
   * When the gadget is running in debug mode, displays the string Message in
   * the debug console.
   */
  virtual void DebugTrace(const char *message) = 0;

  /**
   * When the gadget is running in debug mode, displays the string Message in
   * the debug console as a warning.
   */
  virtual void DebugWarning(const char *message) = 0;

};

} // namespace ggadget

#endif // GGADGET_GADGET_INTERFACE_H__
