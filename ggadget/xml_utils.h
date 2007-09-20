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

#include "string_utils.h"

namespace ggadget {

class ViewInterface;
class ElementInterface;
class Elements;

/**
 * Sets up a view by parsing xml content.
 * @param view a newly created blank view.
 * @param xml the content of an XML file.
 * @param filename the name of the XML file (only for logging).
 * @return @c true if XML parsing succeeds. Errors during view/element
 *     hierarchy setup are only logged. 
 */
bool SetupViewFromXML(ViewInterface *view,
                      const char *xml,
                      const char *filename);

/**
 * Creates an element according to XML definition and appends it to elements.
 * @param elements the elements collection.
 * @param xml the XML definition of the element.
 */
ElementInterface *AppendElementFromXML(Elements *elements, const char *xml);

/**
 * Creates an element according to XML definition and inserts it to elements.
 * @param elements the elements collection.
 * @param xml the XML definition of the element.
 * @param before insert the new element before this element.
 */
ElementInterface *InsertElementFromXML(Elements *elements,
                                       const char *xml,
                                       const ElementInterface *before);

/**
 * Parses a XML file and store the result into a string map.
 * 
 * The string map acts like a simple DOM that supporting XPath like queries.
 * When a key is given:
 *   - element_name: retreives the text content of the second level element
 *     named 'element_name' (the root element name is omitted);
 *   - element_name/subele_name: retrieves the text content of the third level
 *     element named 'subele_name' under the second level element named
 *     'element_name';
 *   - @attr_name: retrives the value of attribute named 'attr_name' in the
 *     top level element; 
 *   - element_name@attr_name: retrieves the value of attribute named
 *     'attr_name' in the secondd level element named 'element_name'.
 *
 * If there are multiple elements with the same name under the same element,
 * the name of the elements from the second one will be postpended with "[n]"
 * where 'n' is the sequence of the element in the elements with the same name
 * (count from 1). 
 *
 * @param xml the content of an XML file.
 * @param filename the name of the XML file (only for logging).
 * @param root_element_name expected name of the root element. 
 * @param table the string table to fill.
 * @return @c true if succeeds.
 */
bool ParseXMLIntoXPathMap(const char *xml, const char *filename,
                          const char *root_element_name,
                          GadgetStringMap *table);

} // namespace ggadget

#endif // GGADGET_XML_UTILS_H__
