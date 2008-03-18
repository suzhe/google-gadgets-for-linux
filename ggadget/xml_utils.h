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

class BasicElement;
class Elements;
class DOMDocumentInterface;
class DOMElementInterface;
class ScriptableInterface;
class ScriptContextInterface;

/**
 * Sets up properties of a Scriptable instance from a specified DOMElement.
 *
 * @param scriptable the Scriptable instance to be setup.
 * @param script_context the ScriptContext instance to be used to execute
 *        script codes. Could be NULL, then all script properties won't be set.
 * @param xml_element DOMElement instance that contains descriptions of all
 *        properties.
 * @param filename file name from which the xml content was loaded, for logging
 *        purpose.
 */
void SetupScriptableProperties(ScriptableInterface *scriptable,
                               ScriptContextInterface *script_context,
                               const DOMElementInterface *xml_element,
                               const char *filename);

/**
 * Creates an element according to a DOMElement instance and inserts it to
 * elements.
 *
 * @param elements the elements collection.
 * @param script_context the ScriptContext instance to be used to execute
 *        script codes. Could be NULL, then all script properties won't be set.
 * @param xml_element the DOMElement that contains definition of the element.
 * @param before insert the new element before this element. If it's NULL, then
 *        append the new element to the end of elements.
 * @param filename file name from which the xml content was loaded, for logging
 *        purpose.
 */
BasicElement *InsertElementFromDOM(Elements *elements,
                                   ScriptContextInterface *script_context,
                                   const DOMElementInterface *xml_element,
                                   const BasicElement *before,
                                   const char *filename);

/**
 * Creates an element according to XML definition and inserts it to elements.
 *
 * @param elements the elements collection.
 * @param script_context the ScriptContext instance to be used to execute
 *        script codes. Could be NULL, then all script properties won't be set.
 * @param xml the XML definition of the element.
 * @param before insert the new element before this element. If it's NULL, then
 *        append the new element to the end of elements.
 */
BasicElement *InsertElementFromXML(Elements *elements,
                                   ScriptContextInterface *script_context,
                                   const std::string &xml,
                                   const BasicElement *before);

/**
 * Replaces all entities in a specified xml content to corresponding
 * definitions provided in a specified GadgetStringMap object.
 *
 * All entities that don't have definitions will not be replaced.
 *
 * @param entities a GadgetStringMap object containing definitions of all
 *        entities.
 * @param[in/out] xml the xml content in which all entities will be replaced.
 * @return false if there is any error (eg. unterminated entity reference) in
 *         the xml content. The original xml content won't be changed when
 *         returning false.
 */
bool ReplaceXMLEntities(const GadgetStringMap &entities, std::string *xml);

} // namespace ggadget

#endif // GGADGET_XML_UTILS_H__
