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

class ViewInterface;
class ElementInterface;
class Elements;
class DOMDocumentInterface;

/**
 * Sets up a view by loading xml content from a file and parsing it.
 * @param view a newly created blank view.
 * @param filename the name of the XML file (relative to the gadget base path).
 * @return @c true if loading and XML parsing succeeds. Errors during
 *     view/element hierarchy setup are only logged.
 */
bool SetupViewFromFile(ViewInterface *view, const char *filename);

/**
 * Sets up a view by parsing xml content.
 * @param view a newly created blank view.
 * @param xml the xml content.
 * @param filename the name of the XML file (only for logging).
 * @return @c true if XML parsing succeeds. Errors during view/element
 *     hierarchy setup are only logged.
 */
bool SetupViewFromXML(ViewInterface *view, const char *xml,
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
 * Parses an XML file and store the result into a string map.
 *
 * The string map acts like a simple DOM that supporting XPath like queries.
 * When a key is given:
 *   - element_name: retreives the text content of the second level element
 *     named 'element_name' (the root element name is omitted);
 *   - element_name/subele_name: retrieves the text content of the third level
 *     element named 'subele_name' under the second level element named
 *     'element_name';
 *   - @@attr_name: retrives the value of attribute named 'attr_name' in the
 *     top level element;
 *   - element_name@@attr_name: retrieves the value of attribute named
 *     'attr_name' in the second level element named 'element_name'.
 *
 * If there are multiple elements with the same name under the same element,
 * the name of the elements from the second one will be postpended with "[n]"
 * where 'n' is the sequence of the element in the elements with the same name
 * (count from 1).
 *
 * @param xml the content of an XML file.
 * @param filename the name of the XML file (only for logging).
 * @param root_element_name expected name of the root element.
 * @param[in, out] encoding on input, it is the hint encoding if the input xml
 *     has no Unicode BOF or xml encoding declaration. On output, it contains
 *     the detected encoding. Can be @c NULL if the caller doesn't need it.
 * @param table the string table to fill.
 * @return @c true if succeeds.
 */
bool ParseXMLIntoXPathMap(const char *xml, const char *filename,
                          const char *root_element_name,
                          std::string *encoding,
                          GadgetStringMap *table);

/**
 * Checks if an XML name is a valid name.
 * @param name the XML name in UTF8 encoding.
 * @return @c true if the name is valid.
 */
bool CheckXMLName(const char *name);

/**
 * Parses XML and build the DOM tree.
 * @param xml the content of an XML file.
 * @param filename the name of the XML file (only for logging).
 * @param is_html @c true if the input is an HTML document.
 * @param domdoc the DOM document. It must be blank before calling this
 *     function, and will be filled with DOM data if this function succeeds.
 * @param[in, out] encoding on input, it is the hint encoding if the input xml
 *     has no Unicode BOF or xml encoding declaration. On output, it contains
 *     the detected encoding. Can be @c NULL if the caller doesn't need it.
 * @return @c true if succeeds.
 */
bool ParseXMLIntoDOM(const char *xml, const char *filename,
                     DOMDocumentInterface *domdoc,
                     std::string *encoding);

/**
 * Parses HTML and build the DOM tree.
 */
bool ParseHTMLIntoDOM(const char *html, const char *filename,
                      DOMDocumentInterface *domdoc,
                      std::string *encoding);

/**
 * Converts a string in given encoding to a utf8. Conversion will be only taken
 * when it's confident, i.e., if Unicode BOF exists, or successfully converted
 * using the given encoding.
 *
 * @param src the string to be converted.
 * @param src_length length of the source string.
 * @param[in, out] encoding on input if it is not empty, it will be used in
 *     convertion, otherwise the encoding will be detected with BOF.
 *     On output, it contains the detected encoding.
 *     Can be @c NULL if the caller doesn't need it.
 * @param[out] dest result utf8 string.
 * @return @c true if it has enough information to detect the encoding.
 */
bool ConvertStringToUTF8(const char *src, size_t src_length,
                         std::string *encoding, std::string *dest);

/**
 * Converts a string in given encoding to a utf8 string.
 *
 * Same as above function but takes a std::string object as source.
 */
bool ConvertStringToUTF8(const std::string &src,
                         std::string *encoding, std::string *dest);

/**
 * Encode a string into XML text by escaping special chars.
 */
std::string EncodeXMLString(const char *src);

} // namespace ggadget

#endif // GGADGET_XML_UTILS_H__
