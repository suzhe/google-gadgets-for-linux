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

#include <cstring>
#include <cmath>
#include <libxml/encoding.h>
#include <libxml/parser.h>
// For xmlCreateMemoryParserCtxt and xmlParseName.
#include <libxml/parserInternals.h>
#include <libxml/tree.h>

#include "xml_utils.h"
#include "common.h"
#include "element_interface.h"
#include "elements.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "script_context_interface.h"
#include "unicode_utils.h"
#include "view_interface.h"
#include "xml_dom.h"

namespace ggadget {

static inline char *FromXmlCharPtr(xmlChar *xml_char_ptr) {
  return reinterpret_cast<char *>(xml_char_ptr);
}

static inline const char *FromXmlCharPtr(const xmlChar *xml_char_ptr) {
  return reinterpret_cast<const char *>(xml_char_ptr);
}

static inline const xmlChar *ToXmlCharPtr(const char *char_ptr) {
  return reinterpret_cast<const xmlChar *>(char_ptr);
}
// There is no non-const version of ToXmlChar *because it's not allowed to
// transfer a non-const char * to libxml.

static xmlDoc *ParseXML(const char *xml, const char *filename,
                        std::string *encoding) {
  if (encoding)
    encoding->clear();

  xmlParserCtxt *ctxt = xmlCreateMemoryParserCtxt(xml, strlen(xml));
  if (!ctxt)
    return NULL;

  // Let the built-in libxml2 error reporter print the correct filename.
  ctxt->input->filename = xmlMemStrdup(filename);

  xmlParseDocument(ctxt);
  xmlDoc *result = NULL;
  if (ctxt->wellFormed) {
    result = ctxt->myDoc;
    if (encoding)
      *encoding = FromXmlCharPtr(ctxt->input->encoding);
  } else {
    xmlFreeDoc(ctxt->myDoc);
    ctxt->myDoc = NULL;
  }
  xmlFreeParserCtxt(ctxt);

  return result;
}

static xmlDoc *ParseHTML(const char *html, const char *filename,
                         std::string *encoding) {
  if (encoding)
    encoding->clear();

  htmlParserCtxt *ctxt = htmlCreateMemoryParserCtxt(html, strlen(html));
  if (!ctxt)
    return NULL;

  // Let the built-in libxml2 error reporter print the correct filename.
  ctxt->input->filename = xmlMemStrdup(filename);
    
  htmlParseDocument(ctxt);
  xmlDoc *result = NULL;
  if (ctxt->wellFormed) {
    result = ctxt->myDoc;
    if (encoding)
      *encoding = FromXmlCharPtr(ctxt->input->encoding);
  } else {
    xmlFreeDoc(ctxt->myDoc);
    ctxt->myDoc = NULL;
  }
  htmlFreeParserCtxt(ctxt);

  return result;
}

static bool ParseBoolValue(const char *value, bool *result) {
  if (strcasecmp("true", value) == 0) {
    *result = true;
    return true;
  }
  if (strcasecmp("false", value) == 0) {
    *result = false;
    return true;
  }
  return false;
}

static bool ParseDoubleValue(const char *value, double *result) {
  char *end_ptr;
  *result = strtod(value, &end_ptr);
  if (*value == '\0' || *end_ptr != '\0' ||
      // We don't allow hexidecimal numbers.
      strchr(value, 'x') || strchr(value, 'X') ||
      // We don't allow INFINITY or NAN.
      strchr(value, 'n') || strchr(value, 'N')) {
    return false;
  }
  return true;
}

static void SetScriptableProperty(ScriptableInterface *scriptable,
                                  ScriptContextInterface *script_context,
                                  const char *filename,
                                  int row,
                                  const char *name,
                                  const char *value,
                                  const char *tag_name) {
  int id;
  Variant prototype;
  bool is_method;
  bool result = scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method);
  if (!result || is_method ||
      id == ScriptableInterface::ID_CONSTANT_PROPERTY ||
      id == ScriptableInterface::ID_DYNAMIC_PROPERTY) {
    LOG("%s:%d: Can't set property %s for %s", filename, row, name, tag_name);
    return;
  }

  Variant property_value;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL: {
      bool b;
      if (ParseBoolValue(value, &b)) {
        property_value = Variant(b);
      } else {
        LOG("%s:%d: Invalid bool '%s' for property %s of %s",
            filename, row, value, name, tag_name);
        property_value = Variant(GadgetStrCmp("true", value) == 0 ?
                               true : false);
        return;
      }
      break;
    }
    case Variant::TYPE_INT64:
    case Variant::TYPE_DOUBLE: {
      double d;
      if (ParseDoubleValue(value, &d)) {
        property_value = prototype.type() == Variant::TYPE_INT64 ?
                         Variant(static_cast<int64_t>(round(d))) :
                         Variant(d);
      } else {
        LOG("%s:%d: Invalid integer '%s' for property %s of %s",
            filename, row, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_STRING:
      property_value = Variant(std::string(value));
      break;

    case Variant::TYPE_VARIANT: {
      double d;
      bool b;
      if (ParseDoubleValue(value, &d)) {
        // '5.0' should be converted to a double instead of integer.
        property_value = strchr(value, '.') || round(d) != d ?
                         Variant(d) : Variant(static_cast<int64_t>(d));
      } else if (ParseBoolValue(value, &b)) {
        property_value = Variant(b);
      } else {
        property_value = Variant(std::string(value));
      }
      break;
    }
    case Variant::TYPE_SLOT:
      property_value = Variant(script_context->Compile(value, filename, row));
      break;

    default:
      LOG("%s:%d: Unsupported type %s when setting property %s for %s",
          filename, row, prototype.ToString().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("%s:%d: Can't set readonly property %s for %s",
        filename, row, name, tag_name);
}

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      ScriptContextInterface *script_context,
                                      const char *filename,
                                      xmlNode *xml_element) {
  const char *tag_name = FromXmlCharPtr(xml_element->name);
  for (xmlAttr *attribute = xml_element->properties;
       attribute != NULL; attribute = attribute->next) {
    const char *name = FromXmlCharPtr(attribute->name);
    char *value = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(attribute)));

    if (GadgetStrCmp(kInnerTextProperty, name) == 0) {
      LOG("%s is not allowed in XML as an attribute: ", kInnerTextProperty);
      continue;
    }

    SetScriptableProperty(scriptable, script_context,
                          filename, xml_element->line,
                          name, value, tag_name);
    if (value)
      xmlFree(value);
  }

  // Set the "innerText" property.
  char *text = FromXmlCharPtr(xmlNodeGetContent(xml_element));
  if (text) {
    std::string trimmed_text = TrimString(std::string(text));
    if (!trimmed_text.empty()) {
      SetScriptableProperty(scriptable, script_context,
                            filename, xml_element->line, kInnerTextProperty,
                            trimmed_text.c_str(), tag_name);
    }
    xmlFree(text);
  }
}

static void HandleScriptElement(ScriptContextInterface *script_context,
                                FileManagerInterface *file_manager,
                                const char *filename,
                                xmlNode *xml_element) {
  char *src = FromXmlCharPtr(xmlGetProp(xml_element, ToXmlCharPtr(kSrcAttr)));
  int lineno = xml_element->line;
  std::string script;
  std::string real_path;
  if (src) {
    if (file_manager->GetFileContents(src, &script, &real_path)) {
      filename = real_path.c_str();
      lineno = 1;
    }
  } else {
    // Uses the Windows version convention, that inline scripts should be
    // quoted in comments.
    for (xmlNode *child = xml_element->children;
         child != NULL; child = child->next) {
      char *content = FromXmlCharPtr(xmlNodeGetContent(child));
      if (content) {
        script = content;
        xmlFree(content);
      }

      if (child->type == XML_COMMENT_NODE)
        break;

      // Other contents are not allowed under <script></script>.
      if (child->type != XML_TEXT_NODE || !TrimString(script).empty()) {
        LOG("%s:%d: This content is not allowed in script element",
            filename, xml_element->line);
        break;
      }
    }
  }

  if (!script.empty())
    script_context->Execute(script.c_str(), filename, lineno);
}

static void HandleAllScriptElements(ViewInterface *view,
                                    const char *filename,
                                    xmlNode *xml_element) {
  xmlNode *child = xml_element->children;
  while (child) {
    if (child->type != XML_ELEMENT_NODE) {
      child = child->next;
    } else if (GadgetStrCmp(FromXmlCharPtr(child->name), kScriptTag) == 0) {
      HandleScriptElement(view->GetScriptContext(),
                          view->GetFileManager(),
                          filename, child);
      // Remove the script node to prevent further processing.
      xmlNode *temp = child;
      child = child->next;
      xmlUnlinkNode(temp);
      xmlFreeNode(temp);
    } else {
      HandleAllScriptElements(view, filename, child);
      child = child->next;
    }
  }
}

static ElementInterface *InsertElementFromDOM(Elements *elements,
                                              const char *filename,
                                              xmlNode *xml_element,
                                              const ElementInterface *before) {
  const char *tag_name = FromXmlCharPtr(xml_element->name);
  char *name = NULL;
  xmlAttr *nameAttr = xmlHasProp(xml_element, ToXmlCharPtr(kNameAttr));
  if (nameAttr) {
    name = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(nameAttr)));
  }

  ElementInterface *element = elements->InsertElement(tag_name, before, name);
  // Remove the "name" attribute to prevent further processing.
  if (nameAttr) {
    xmlRemoveProp(nameAttr);
    if (name)
      xmlFree(name);
  }

  if (!element) {
    LOG("%s:%d: Failed to create element %s",
        filename, xml_element->line, tag_name);
    return element;
  }

  SetupScriptableProperties(element, element->GetView()->GetScriptContext(),
                            filename, xml_element);
  Elements *children = element->GetChildren();
  for (xmlNode *child = xml_element->children;
       child != NULL; child = child->next) {
    if (child->type == XML_ELEMENT_NODE)
      InsertElementFromDOM(children, filename, child, NULL);
  }
  return element;
}

bool SetupViewFromFile(ViewInterface *view, const char *filename) {
  std::string contents;
  std::string real_path;
  if (!view->GetFileManager()->GetXMLFileContents(filename,
                                                  &contents, &real_path))
    return false;

  return SetupViewFromXML(view, contents.c_str(), real_path.c_str());
}

bool SetupViewFromXML(ViewInterface *view, const char *xml,
                      const char *filename) {
  xmlDoc *xmldoc = ParseXML(xml, filename, NULL);
  if (!xmldoc)
    return false;

  xmlNode *view_element = xmlDocGetRootElement(xmldoc);
  if (!view_element ||
      GadgetStrCmp(FromXmlCharPtr(view_element->name), kViewTag) != 0) {
    LOG("No valid root element in view file: %s", filename);
    xmlFreeDoc(xmldoc);
    return false;
  }

  // <script> elements should be handled first because later parsing needs
  // the script context.
  HandleAllScriptElements(view, filename, view_element);
  SetupScriptableProperties(view, view->GetScriptContext(),
                            filename, view_element);
  Elements *children = view->GetChildren();
  for (xmlNode *child = view_element->children;
       child != NULL; child = child->next) {
    if (child->type == XML_ELEMENT_NODE)
      InsertElementFromDOM(children, filename, child, NULL);
  }

  xmlFreeDoc(xmldoc);
  return true;
}

ElementInterface *AppendElementFromXML(Elements *elements, const char *xml) {
  return InsertElementFromXML(elements, xml, NULL);
}

ElementInterface *InsertElementFromXML(Elements *elements,
                                       const char *xml,
                                       const ElementInterface *before) {
  xmlDoc *xmldoc = ParseXML(xml, xml, NULL);
  if (!xmldoc)
    return NULL;

  xmlNode *xml_element = xmlDocGetRootElement(xmldoc);
  if (!xml_element) {
    LOG("No root element in xml definition: %s", xml);
    xmlFreeDoc(xmldoc);
    return NULL;
  }

  ElementInterface *result = InsertElementFromDOM(elements, "",
                                                  xml_element, before);
  xmlFreeDoc(xmldoc);
  return result;
}

// Count the sequence of a child in the elements of the same tag name.
static int CountTagSequence(const xmlNode *child, const char *tag) {
  int count = 1;
  for (const xmlNode *node = child->prev; node != NULL; node = node->prev) {
    if (node->type == XML_ELEMENT_NODE &&
        GadgetStrCmp(tag, FromXmlCharPtr(node->name)) == 0)
      count++;
  }
  return count;
}

static void ConvertElementIntoXPathMap(const xmlNode *element,
                                       const std::string &prefix,
                                       GadgetStringMap *table) {
  for (xmlAttr *attribute = element->properties;
       attribute != NULL; attribute = attribute->next) {
    const char *name = FromXmlCharPtr(attribute->name);
    char *value = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(attribute)));
    (*table)[prefix + '@' + name] = std::string(value ? value : "");
    if (value)
      xmlFree(value);
  }

  for (xmlNode *child = element->children; child != NULL; child = child->next) {
    if (child->type == XML_ELEMENT_NODE) {
      const char *tag = FromXmlCharPtr(child->name);
      char *text = FromXmlCharPtr(xmlNodeGetContent(child));
      std::string key(prefix);
      if (!prefix.empty()) key += '/';
      key += tag;

      if (table->find(key) != table->end()) {
        // Postpend the sequence if there are multiple elements with the same
        // name.
        char buf[20];
        snprintf(buf, sizeof(buf), "[%d]", CountTagSequence(child, tag));
        key += buf;
      }
      (*table)[key] = text ? text : "";
      if (text) xmlFree(text);

      ConvertElementIntoXPathMap(child, key, table);
    }
  }
}

bool ParseXMLIntoXPathMap(const char *xml, const char *filename,
                          const char *root_element_name,
                          GadgetStringMap *table) {
  xmlDoc *xmldoc = ParseXML(xml, filename, NULL);
  if (!xmldoc)
    return false;

  xmlNode *root = xmlDocGetRootElement(xmldoc);
  if (!root ||
      GadgetStrCmp(FromXmlCharPtr(root->name), root_element_name) != 0) {
    LOG("No valid root element %s in XML file: %s",
        root_element_name, filename);
    xmlFreeDoc(xmldoc);
    return false;
  }

  ConvertElementIntoXPathMap(root, "", table);
  xmlFreeDoc(xmldoc);
  return true;
}

bool CheckXMLName(const char *name) {
  if (!name || !*name)
    return false;

  xmlParserCtxt *ctxt = xmlCreateMemoryParserCtxt(name, strlen(name));
  if (ctxt) {
    const char *result = FromXmlCharPtr(xmlParseName(ctxt));
    bool succeeded = result && strcmp(result, name) == 0;
    xmlFreeParserCtxt(ctxt);
    return succeeded;
  }
  return false;
}

static void ConvertCharacterDataIntoDOM(DOMDocumentInterface *domdoc,
                                        DOMNodeInterface *parent,
                                        xmlNode *xmltext) {
  char *text = FromXmlCharPtr(xmlNodeGetContent(xmltext));
  UTF16String utf16_text;
  if (text) {
    std::string text_str(text);
    ConvertStringUTF8ToUTF16(text_str, &utf16_text);
    xmlFree(text);
  }

  DOMCharacterDataInterface *data = NULL;
  switch (xmltext->type) {
    case XML_TEXT_NODE:
      // Don't create empty text nodes.
      if (!utf16_text.empty())
        data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case XML_ENTITY_REF_NODE:
      data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case XML_CDATA_SECTION_NODE:
      data = domdoc->CreateCDATASection(utf16_text.c_str());
      break;
    case XML_COMMENT_NODE:
      data = domdoc->CreateComment(utf16_text.c_str());
      break;
    default:
      ASSERT(false);
      break;
  }
  if (data)
    parent->AppendChild(data);
}

static void ConvertPIIntoDOM(DOMDocumentInterface *domdoc,
                             DOMNodeInterface *parent,
                             xmlNode *xmlpi) {
  const char *target = FromXmlCharPtr(xmlpi->name);
  char *data = FromXmlCharPtr(xmlNodeGetContent(xmlpi));
  DOMProcessingInstructionInterface *pi;
  domdoc->CreateProcessingInstruction(target, data, &pi);
  if (pi)
    parent->AppendChild(pi);
  if (data)
    xmlFree(data);
}

static void ConvertElementIntoDOM(DOMDocumentInterface *domdoc,
                                  DOMNodeInterface *parent,
                                  xmlNode *xmlele);

static void ConvertChildrenIntoDOM(DOMDocumentInterface *domdoc,
                                   DOMNodeInterface *parent,
                                   xmlNode *xmlnode) {
  for (xmlNode *child = xmlnode->children; child != NULL; child = child->next) {
    switch (child->type) {
      case XML_ELEMENT_NODE:
        ConvertElementIntoDOM(domdoc, parent, child);
        break;
      case XML_TEXT_NODE:
      case XML_ENTITY_REF_NODE:
      case XML_CDATA_SECTION_NODE:
      case XML_COMMENT_NODE:
        ConvertCharacterDataIntoDOM(domdoc, parent, child);
        break;
      case XML_PI_NODE:
        ConvertPIIntoDOM(domdoc, parent, child);
        break;
      default:
        DLOG("Ignore XML Node of type %d\n", child->type);
        break;
    }
  }
}

static void ConvertElementIntoDOM(DOMDocumentInterface *domdoc,
                                  DOMNodeInterface *parent,
                                  xmlNode *xmlele) {
  DOMElementInterface *element;
  domdoc->CreateElement(FromXmlCharPtr(xmlele->name), &element);
  if (!element || DOM_NO_ERR != parent->AppendChild(element)) {
    // Unlikely to happen.
    DLOG("Failed to create DOM element or to add it to parent");
    delete element;
    return;
  }

  for (xmlAttr *attribute = xmlele->properties;
       attribute != NULL; attribute = attribute->next) {
    const char *name = FromXmlCharPtr(attribute->name);
    char *value = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(attribute)));
    element->SetAttribute(name, value);
    if (value)
      xmlFree(value);
  }

  ConvertChildrenIntoDOM(domdoc, element, xmlele);
}

bool ParseXMLIntoDOM(const char *xml, const char *filename,
                     DOMDocumentInterface *domdoc,
                     std::string *encoding) {
  ASSERT(domdoc);
  if (domdoc->HasChildNodes())
    return false;

  xmlDoc *xmldoc = ParseXML(xml, filename, encoding);
  if (!xmldoc)
    return false;

  if (!xmlDocGetRootElement(xmldoc)) {
    LOG("No root element in XML file: %s", filename);
    xmlFreeDoc(xmldoc);
    return false;
  }

  ConvertChildrenIntoDOM(domdoc, domdoc, reinterpret_cast<xmlNode *>(xmldoc));
  domdoc->Normalize();

  xmlFreeDoc(xmldoc);
  return true;
}

bool ParseHTMLIntoDOM(const char *html, const char *filename,
                      DOMDocumentInterface *domdoc,
                      std::string *encoding) {
  ASSERT(domdoc);
  if (domdoc->HasChildNodes())
    return false;

  xmlDoc *xmldoc = ParseHTML(html, filename, encoding);
  if (!xmldoc)
    return false;

  if (!xmlDocGetRootElement(xmldoc)) {
    LOG("No root element in XML file: %s", filename);
    xmlFreeDoc(xmldoc);
    return false;
  }

  ConvertChildrenIntoDOM(domdoc, domdoc, reinterpret_cast<xmlNode *>(xmldoc));
  domdoc->Normalize();

  xmlFreeDoc(xmldoc);
  return true;
}

bool ConvertStringToUTF8(const char *src, size_t src_length,
                         const char *encoding, std::string *dest) {
  if (!src || !dest)
    return false;

  dest->clear();
  if (!src_length)
    return true;

  // Check if src_length can be stored into an int variable to prevent overflow
  // in libxml2.
  if (static_cast<int>(src_length) < 0)
    return false;

  xmlCharEncodingHandler *encoding_handler = NULL;
  if (encoding && *encoding) {
    encoding_handler = xmlFindCharEncodingHandler(encoding);
  } else {
    // xmlDetectCharEncoding detects encoding by looking at BOM.
    xmlCharEncoding xml_encoding = xmlDetectCharEncoding(ToXmlCharPtr(src),
                                                         src_length);
    encoding_handler = xmlGetCharEncodingHandler(xml_encoding);
  }

  if (!encoding_handler) {
    // libxml2 returns NULL when either the encoding is unknown or it thinks the
    // source string doesn't need to be converted.
    if (IsLegalUTF8String(src, src_length)) {
      dest->assign(src, src_length);
      return true;
    }
    return false;
  }

  xmlBuffer *input_buffer = xmlBufferCreateStatic(const_cast<char *>(src),
                                                  src_length);
  xmlBuffer *output_buffer = xmlBufferCreate();
  int result = xmlCharEncInFunc(encoding_handler, output_buffer, input_buffer);
  if (result > 0) {
    ASSERT(result == xmlBufferLength(output_buffer));
    dest->append(FromXmlCharPtr(xmlBufferContent(output_buffer)),
                 static_cast<size_t>(result));
  }

  xmlCharEncCloseFunc(encoding_handler);
  xmlBufferFree(input_buffer);
  xmlBufferFree(output_buffer);
  return result >= 0;
}

bool ConvertStringToUTF8(const std::string &src, const char *encoding,
                         std::string *dest) {
  return ConvertStringToUTF8(src.c_str(), src.length(), encoding, dest);
}

} // namespace ggadget
