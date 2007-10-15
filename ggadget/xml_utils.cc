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
#include "xml_utils.h"
#include "common.h"
#include "element_interface.h"
#include "elements.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "script_context_interface.h"
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
                                  int row, int column,
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
    LOG("%s:%d:%d: Can't set property %s from xml for %s",
        filename, row, column, name, tag_name);
    return;
  }

  Variant property_value;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL: {
      bool b;
      if (ParseBoolValue(value, &b)) {
        property_value = Variant(b);
      } else {
        LOG("%s:%d:%d: Invalid bool '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
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
        LOG("%s:%d:%d: Invalid integer '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
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
      LOG("%s:%d:%d: Unsupported type %s when setting property %s for %s",
          filename, row, column, prototype.ToString().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("%s:%d:%d: Can't set readonly property %s for %s",
        filename, row, column, name, tag_name);
}

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      ScriptContextInterface *script_context,
                                      const char *filename,
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

    SetScriptableProperty(scriptable, script_context,
                          filename, attribute->Row(), attribute->Column(),
                          name, value, tag_name);
  }

  // Set the "innerText" property.
  const char *text = xml_element->GetText();
  if (text)
    SetScriptableProperty(scriptable, script_context,
                          filename, xml_element->Row(), xml_element->Column(),
                          kInnerTextProperty, text, tag_name);
}

static void HandleScriptElement(ScriptContextInterface *script_context,
                                FileManagerInterface *file_manager,
                                const char *filename,
                                TiXmlElement *xml_element) {
  const char *src = xml_element->Attribute(kSrcAttr);
  const char *script = NULL;
  int lineno = xml_element->Row();
  std::string data;
  std::string real_path;
  if (src) {
    if (file_manager->GetFileContents(src, &data, &real_path)) {
      filename = real_path.c_str();
      lineno = 1;
      script = data.c_str();
    }
  } else {
    for (TiXmlNode* child = xml_element->FirstChild();
         child != NULL; child = child->NextSibling()) {
      TiXmlComment *comment = child->ToComment();
      if (comment) {
        script = comment->Value();
        break;
      }
    }

    if (!script)
      LOG("%s:%d:%d: Either 'src' attribute or script in XML comment needed",
          filename, xml_element->Row(), xml_element->Column());
  }

  if (script)
    script_context->Execute(script, filename, lineno);
}

static void HandleAllScriptElements(ViewInterface *view,
                                    const char *filename,
                                    TiXmlElement *xml_element) {
  TiXmlElement *child = xml_element->FirstChildElement();
  while (child) {
    if (GadgetStrCmp(child->Value(), kScriptTag) == 0) {
      HandleScriptElement(view->GetScriptContext(),
                          view->GetFileManager(),
                          filename, child);
      TiXmlElement *temp = child;
      child = child->NextSiblingElement();
      xml_element->RemoveChild(temp);
    } else {
      HandleAllScriptElements(view, filename, child);
      child = child->NextSiblingElement();
    }
  }
}

static ElementInterface *InsertElementFromDOM(Elements *elements,
                                              const char *filename,
                                              TiXmlElement *xml_element,
                                              const ElementInterface *before) {
  const char *tag_name = xml_element->Value();
  const char *name = xml_element->Attribute(kNameAttr);  // May be NULL.
  ElementInterface *element = elements->InsertElement(tag_name, before, name);
  xml_element->RemoveAttribute(kNameAttr);
  if (!element) {
    LOG("%s:%d:%d: Failed to create element %s",
        filename,xml_element->Row(), xml_element->Column(), tag_name);
    return element;
  }

  SetupScriptableProperties(element, element->GetView()->GetScriptContext(),
                            filename, xml_element);
  Elements *children = element->GetChildren();
  for (TiXmlElement *child_xml_ele = xml_element->FirstChildElement();
       child_xml_ele != NULL;
       child_xml_ele = child_xml_ele->NextSiblingElement()) {
    InsertElementFromDOM(children, filename, child_xml_ele, NULL);
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
  TiXmlDocument xmldoc;
  if (!ParseXML(xml, filename, &xmldoc))
    return false;

  TiXmlElement *view_element = xmldoc.RootElement();
  if (!view_element || GadgetStrCmp(view_element->Value(), kViewTag) != 0) {
    LOG("No valid root element in view file: %s", filename);
    return false;
  }

  // <script> elements should be handled first because later parsing needs
  // the script context.
  HandleAllScriptElements(view, filename, view_element);
  SetupScriptableProperties(view, view->GetScriptContext(),
                            filename, view_element);
  Elements *children = view->GetChildren();
  for (TiXmlElement *child_xml_ele = view_element->FirstChildElement();
       child_xml_ele != NULL;
       child_xml_ele = child_xml_ele->NextSiblingElement()) {
    InsertElementFromDOM(children, filename, child_xml_ele, NULL);
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

  return InsertElementFromDOM(elements, "", xml_element, before);
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
