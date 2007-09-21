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
#include "xml_utils.h"
#include "common.h"
#include "element_interface.h"
#include "elements.h"
#include "gadget_consts.h"
#include "third_party/tinyxml/tinyxml.h"
#include "view_interface.h"

namespace ggadget {

static bool ParseXML(const char *xml,
                     const char *filename,
                     TiXmlDocument *xmldoc) {
  xmldoc->Parse(xml);
  if (xmldoc->Error()) {
    LOG("Error parsing xml: %s in %s:%d:%d",
        xmldoc->ErrorDesc(), filename, xmldoc->ErrorRow(), xmldoc->ErrorCol());
    return false;
  }
  return true;
}

static void SetScriptableProperty(ScriptableInterface *scriptable,
                                  const char *name, const char *value,
                                  const char *tag_name) {
  int id;
  Variant prototype;
  bool is_method;
  bool result = scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method);
  if (!result || is_method ||
      id == ScriptableInterface::ID_CONSTANT_PROPERTY ||
      id == ScriptableInterface::ID_DYNAMIC_PROPERTY) {
    LOG("Can't set property %s from xml for %s", name, tag_name);
    return;
  }

  Variant property_value;
  char *end_ptr;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL:
      property_value = Variant(GadgetStrCmp("true", value) == 0 ?
                               true : false);
      break;

    case Variant::TYPE_INT64:
      property_value = Variant(strtoll(value, &end_ptr, 10));
      if (*value == '\0' || *end_ptr != '\0') {
        LOG("Invalid integer '%s' for property %s for %s",
            value, name, tag_name);
        return;
      }
      break;

    case Variant::TYPE_DOUBLE:
      property_value = Variant(strtod(value, &end_ptr));
      if (*value == '\0' || *end_ptr != '\0') {
        LOG("Invalid double '%s' for property %s for %s",
            value, name, tag_name);
        return;
      }
      break;

    case Variant::TYPE_STRING:
      property_value = Variant(std::string(value));
      break;

    case Variant::TYPE_VARIANT:
      property_value = Variant(strtoll(value, &end_ptr, 10));
      if (*value == '\0' || *end_ptr != '\0')
        property_value = Variant(std::string(value));
      break;

    case Variant::TYPE_SLOT:
      // TODO:

    default:
      LOG("Unsupported type %s when setting property %s from xml for %s",
          prototype.ToString().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("Can't set readonly property %s from xml for %s", name, tag_name);
}

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      const TiXmlElement *xml_element) {
  const char *tag_name = xml_element->Value();
  for (const TiXmlAttribute *attribute = xml_element->FirstAttribute();
       attribute != NULL;
       attribute = attribute->Next()) {
    const char *name = attribute->Name();
    const char *value = attribute->Value();
    
    if (GadgetStrCmp(kInnerTextProperty, name) == 0) {
      LOG("%s is not allowed in XML as an attribute: ", kInnerTextProperty);
      continue;
    }
    
    SetScriptableProperty(scriptable, name, value, tag_name);
  }
 
  // Set the "innerText" property.
  const char *text = xml_element->GetText();
  if (text)
    SetScriptableProperty(scriptable, kInnerTextProperty, text, tag_name);
}

static ElementInterface *InsertElementFromDOM(Elements *elements,
                                              TiXmlElement *xml_element,
                                              const ElementInterface *before) {
  const char *tag_name = xml_element->Value();
  const char *name = xml_element->Attribute(kNameAttr);  // May be NULL.
  ElementInterface *element = elements->InsertElement(tag_name, before, name);
  xml_element->RemoveAttribute(kNameAttr);
  if (!element)
    return element;

  SetupScriptableProperties(element, xml_element);
  Elements *children = element->GetChildren();
  for (TiXmlElement *child_xml_ele = xml_element->FirstChildElement();
       child_xml_ele != NULL;
       child_xml_ele = child_xml_ele->NextSiblingElement()) {
    InsertElementFromDOM(children, child_xml_ele, NULL);
  }
  return element;
}

bool SetupViewFromXML(ViewInterface *view,
                      const char *xml,
                      const char *filename) {
  TiXmlDocument xmldoc;
  if (!ParseXML(xml, filename, &xmldoc))
    return false;

  TiXmlElement *view_element = xmldoc.RootElement();
  if (!view_element || strcmp(view_element->Value(), kViewTag) != 0) {
    LOG("No valid root element in view file: %s", filename);
    return false;
  }

  SetupScriptableProperties(view, view_element);
  Elements *children = view->GetChildren();
  for (TiXmlElement *child_xml_ele = view_element->FirstChildElement();
       child_xml_ele != NULL;
       child_xml_ele = child_xml_ele->NextSiblingElement()) { 
    InsertElementFromDOM(children, child_xml_ele, NULL);
  }
  return true;
}

ElementInterface *AppendElementFromXML(Elements *elements, const char *xml) {
  return InsertElementFromXML(elements, xml, NULL);
}

ElementInterface *InsertElementFromXML(Elements *elements,
                                       const char *xml,
                                       const ElementInterface *before) {
  TiXmlDocument xmldoc;
  if (!ParseXML(xml, xml, &xmldoc))
    return NULL;

  TiXmlElement *xml_element = xmldoc.RootElement();
  if (!xml_element) {
    LOG("No root element in xml definition: %s", xml);
    return NULL;
  }

  return InsertElementFromDOM(elements, xml_element, before);
}

// Count the sequence of a child in the elements of the same tag name.
static int CountTagSequence(TiXmlElement *child, const char *tag) {
  int count = 1;
  for (TiXmlNode *node = child->PreviousSibling(tag);
       node != NULL;
       node = child->PreviousSibling(tag)) {
    if (node->ToElement()) count++;
  }
  return count;
}

static void ParseDOMIntoXPathMap(TiXmlElement *element,
                                 const std::string &prefix,
                                 GadgetStringMap *table) {
  for (const TiXmlAttribute *attribute = element->FirstAttribute();
       attribute != NULL;
       attribute = attribute->Next()) {
    const char *name = attribute->Name();
    const char *value = attribute->Value();
    (*table)[prefix + '@' + name] = value;
  }

  for (TiXmlElement *child = element->FirstChildElement();
       child != NULL;
       child = child->NextSiblingElement()) {
    const char *tag = child->Value();
    const char *text = child->GetText();
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

    ParseDOMIntoXPathMap(child, key, table);
  }
}
                                 
bool ParseXMLIntoXPathMap(const char *xml, const char *filename,
                          const char *root_element_name,
                          GadgetStringMap *table) {
  TiXmlDocument xmldoc;
  if (!ParseXML(xml, filename, &xmldoc))
    return false;

  TiXmlElement *root = xmldoc.RootElement();
  if (!root || GadgetStrCmp(root->Value(), root_element_name) != 0) {
    LOG("No valid root element %s in XML file: %s",
        root_element_name, filename);
    return false;
  }

  ParseDOMIntoXPathMap(root, "", table);
  return true;
}

} // namespace ggadget
