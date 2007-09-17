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
#include "third_party/tinyxml/tinyxml.h"
#include "view_interface.h"

namespace ggadget {

// The tag name of the root element in string table files.
const char *const kStringsTag = "strings";
// The tag name of the root element in view file.
const char *const kViewTag = "view";
// The attribute name of the 'name' attribute of elements.
const char *const kNameAttr = "name";

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

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      const TiXmlElement *xml_element) {
  const char *tag_name = xml_element->Value();
  for (const TiXmlAttribute *attribute = xml_element->FirstAttribute();
       attribute != NULL;
       attribute = attribute->Next()) {
    const char *name = attribute->Name();
    const char *value = attribute->Value();
    if (!name || !value)
      continue;

    int id;
    Variant prototype;
    bool is_method;
    bool result = scriptable->GetPropertyInfoByName(name, &id,
                                                    &prototype, &is_method);
    if (!result || is_method ||
        id == ScriptableInterface::ID_CONSTANT_PROPERTY ||
        id == ScriptableInterface::ID_DYNAMIC_PROPERTY) {
      LOG("Can't set property %s from xml for %s", name, tag_name);
      continue;
    }

    Variant property_value;
    char *end_ptr;
    switch (prototype.type()) {
      case Variant::TYPE_BOOL:
        property_value = Variant(strcasecmp("true", attribute->Value()) == 0 ?
                                 true : false);
        break;

      case Variant::TYPE_INT64:
        property_value = Variant(strtoll(value, &end_ptr, 10));
        if (*value == '\0' || *end_ptr != '\0') {
          LOG("Invalid integer '%s' for property %s", value, name);
          continue;
        }
        break;

      case Variant::TYPE_DOUBLE:
        property_value = Variant(strtod(value, &end_ptr));
        if (*value == '\0' || *end_ptr != '\0') {
          LOG("Invalid double '%s' for property %s", value, name);
          continue;
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

      default:
        LOG("Can't set property %s from xml for %s", name, tag_name);
        continue;
    }

    if (!scriptable->SetProperty(id, property_value))
      LOG("Can't set readonly property %s from xml for %s", name, tag_name);
  }
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

#ifndef NDEBUG
  // Check if "tagName" property is properly registered.
  int id;
  Variant v;
  bool is_method;
  ASSERT(element->GetPropertyInfoByName("tagName", &id,
                                        &v, &is_method));
  ASSERT(!is_method);
  v = element->GetProperty(id);
  ASSERT(v.type() == Variant::TYPE_STRING);
  ASSERT(strcmp(VariantValue<const char *>()(v), tag_name) == 0);
#endif

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

bool ParseStringTable(const char *xml, const char *filename,
                      std::map<std::string, std::string> *table) {
  TiXmlDocument xmldoc;
  if (!ParseXML(xml, filename, &xmldoc))
    return false;

  TiXmlElement *strings = xmldoc.RootElement();
  if (!strings || strcmp(strings->Value(), kStringsTag) != 0) {
    LOG("No valid root element in string table file: %s", filename);
    return false;
  }

  for (TiXmlElement *child = strings->FirstChildElement();
       child != NULL;
       child = child->NextSiblingElement()) {
    const char *text = child->GetText();
    (*table)[child->Value()] = text ? text : "";
  }
  return true;
}

} // namespace ggadget
