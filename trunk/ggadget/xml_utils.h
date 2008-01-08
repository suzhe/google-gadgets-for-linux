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

#ifndef GGADGET_XML_UTILS_H__
#define GGADGET_XML_UTILS_H__

#include <ggadget/string_utils.h>

namespace ggadget {

class View;
class BasicElement;
class Elements;
class DOMDocumentInterface;

/**
 * Sets up a view by loading xml content from a file and parsing it.
 * @param view a newly created blank view.
 * @param filename the name of the XML file (relative to the gadget base path).
 * @return @c true if loading and XML parsing succeeds. Errors during
 *     view/element hierarchy setup are only logged.
 */
bool SetupViewFromFile(View *view, const char *filename);

/**
 * Sets up a view by parsing xml content.
 * @param view a newly created blank view.
 * @param xml the xml content.
 * @param filename the name of the XML file (only for logging).
 * @return @c true if XML parsing succeeds. Errors during view/element
 *     hierarchy setup are only logged.
 */
bool SetupViewFromXML(View *view, const std::string &xml,
                      const char *filename);

/**
 * Creates an element according to XML definition and appends it to elements.
 * @param view
 * @param elements the elements collection.
 * @param xml the XML definition of the element.
 */
BasicElement *AppendElementFromXML(View *view, Elements *elements,
                                   const std::string &xml);

/**
 * Creates an element according to XML definition and inserts it to elements.
 * @param view
 * @param elements the elements collection.
 * @param xml the XML definition of the element.
 * @param before insert the new element before this element.
 */
BasicElement *InsertElementFromXML(View *view, Elements *elements,
                                   const std::string &xml,
                                   const BasicElement *before);

} // namespace ggadget

#endif // GGADGET_XML_UTILS_H__
