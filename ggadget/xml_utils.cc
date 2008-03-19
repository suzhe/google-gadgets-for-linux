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
#include "basic_element.h"
#include "elements.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "logger.h"
#include "scriptable_interface.h"
#include "script_context_interface.h"
#include "unicode_utils.h"
#include "view.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"

namespace ggadget {

static void SetScriptableProperty(ScriptableInterface *scriptable,
                                  ScriptContextInterface *script_context,
                                  const char *filename, int row, int column,
                                  const char *name, const char *value,
                                  const char *tag_name) {
  int id;
  Variant prototype;
  bool is_method;
  bool result = scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method);
  if (!result || is_method ||
      id == ScriptableInterface::kConstantPropertyId ||
      id == ScriptableInterface::kDynamicPropertyId) {
    LOG("%s:%d:%d Can't set property %s for %s", filename, row, column,
        name, tag_name);
    return;
  }

  Variant str_value_variant(value);
  Variant property_value;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL: {
      bool b;
      if (str_value_variant.ConvertToBool(&b)) {
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
    case Variant::TYPE_INT64: {
      int64_t i;
      if (str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else {
        LOG("%s:%d:%d: Invalid Integer '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_DOUBLE: {
      double d;
      if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else {
        LOG("%s:%d:%d: Invalid double '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_STRING:
      property_value = str_value_variant;
      break;

    case Variant::TYPE_VARIANT: {
      int64_t i;
      double d;
      bool b;
      if (!*value) {
        property_value = str_value_variant;
      } else if (strchr(value, '.') == NULL &&
                 str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else if (str_value_variant.ConvertToBool(&b)) {
        property_value = Variant(b);
      } else {
        property_value = str_value_variant;
      }
      break;
    }
    case Variant::TYPE_SLOT: {
      if (script_context) {
        property_value = Variant(script_context->Compile(value, filename, row));
        break;
      } else {
        LOG("%s:%d:%d: Can't set script '%s' for property %s of %s: "
            "ScriptContext is not available.",
            filename, row, column, value, name, tag_name);
        return;
      }
    }

    default:
      LOG("%s:%d:%d: Unsupported type %s when setting property %s for %s",
          filename, row, column, prototype.Print().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("%s:%d:%d: Can't set readonly property %s for %s",
        filename, row, column, name, tag_name);
}

void SetupScriptableProperties(ScriptableInterface *scriptable,
                               ScriptContextInterface *script_context,
                               const DOMElementInterface *xml_element,
                               const char *filename) {
  std::string tag_name = xml_element->GetTagName();
  const DOMNamedNodeMapInterface *attributes = xml_element->GetAttributes();
  size_t length = attributes->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMAttrInterface *attr = down_cast<const DOMAttrInterface *>(
        attributes->GetItem(i));
    std::string name = attr->GetName();
    std::string value = attr->GetValue();
    if (GadgetStrCmp(kInnerTextProperty, name.c_str()) == 0) {
      LOG("%s is not allowed in XML as an attribute: ", kInnerTextProperty);
      continue;
    }

    if (GadgetStrCmp(kNameAttr, name.c_str()) != 0) {
      SetScriptableProperty(scriptable, script_context, filename,
                            attr->GetRow(), attr->GetColumn(),
                            name.c_str(), value.c_str(), tag_name.c_str());
    }
  }
  delete attributes;
  // "innerText" property is set in InsertElementFromDOM().
}

BasicElement *InsertElementFromDOM(Elements *elements,
                                   ScriptContextInterface *script_context,
                                   const DOMElementInterface *xml_element,
                                   const BasicElement *before,
                                   const char *filename) {
  std::string tag_name = xml_element->GetTagName();
  if (GadgetStrCmp(tag_name.c_str(), kScriptTag) == 0)
    return NULL;

  std::string name = xml_element->GetAttribute(kNameAttr);
  BasicElement *element = elements->InsertElement(tag_name.c_str(), before,
                                                  name.c_str());
  if (!element) {
    LOG("%s:%d:%d: Failed to create element %s", filename,
        xml_element->GetRow(), xml_element->GetColumn(), tag_name.c_str());
    return element;
  }

  SetupScriptableProperties(element, script_context, xml_element, filename);
  Elements *children = element->GetChildren();
  std::string text;
  for (const DOMNodeInterface *child = xml_element->GetFirstChild();
       child; child = child->GetNextSibling()) {
    DOMNodeInterface::NodeType type = child->GetNodeType();
    if (type == DOMNodeInterface::ELEMENT_NODE && children) {
      InsertElementFromDOM(children, script_context,
                           down_cast<const DOMElementInterface *>(child),
                           NULL, filename);
    } else if (type == DOMNodeInterface::TEXT_NODE ||
               type == DOMNodeInterface::CDATA_SECTION_NODE) {
      text += down_cast<const DOMTextInterface *>(child)->GetTextContent();
    }
  }
  // Set the "innerText" property.
  text = TrimString(text);
  if (!text.empty()) {
    SetScriptableProperty(element, script_context, filename,
                          xml_element->GetRow(), xml_element->GetColumn(),
                          kInnerTextProperty, text.c_str(), tag_name.c_str());
  }
  return element;
}

BasicElement *InsertElementFromXML(Elements *elements,
                                   const StringMap *strings,
                                   ScriptContextInterface *script_context,
                                   const std::string &xml,
                                   const BasicElement *before) {
  DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->Ref();
  if (!GetXMLParser()->ParseContentIntoDOM(xml, strings, xml.c_str(),
                                           NULL, NULL, kEncodingFallback,
                                           xmldoc, NULL, NULL)) {
    xmldoc->Unref();
    return false;
  }

  DOMElementInterface *xml_element = xmldoc->GetDocumentElement();
  if (!xml_element) {
    LOG("No root element in xml definition: %s", xml.c_str());
    xmldoc->Unref();
    return NULL;
  }

  BasicElement *result = InsertElementFromDOM(elements, script_context,
                                              xml_element, before, "");
  xmldoc->Unref();
  return result;
}

} // namespace ggadget
