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
#include "script_context_interface.h"
#include "unicode_utils.h"
#include "view_interface.h"
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
      if (strchr(value, '.') == NULL && str_value_variant.ConvertToInt64(&i)) {
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
    case Variant::TYPE_SLOT:
      property_value = Variant(script_context->Compile(value, filename, row));
      break;

    default:
      LOG("%s:%d:%d: Unsupported type %s when setting property %s for %s",
          filename, row, column, prototype.Print().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("%s:%d:%d: Can't set readonly property %s for %s",
        filename, row, column, name, tag_name);
}

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      ScriptContextInterface *script_context,
                                      const char *filename,
                                      const DOMElementInterface *xml_element) {
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

static void HandleScriptElement(ScriptContextInterface *script_context,
                                FileManagerInterface *file_manager,
                                const char *filename,
                                const DOMElementInterface *xml_element) {
  int lineno = xml_element->GetRow();
  std::string script;
  std::string src = xml_element->GetAttribute(kSrcAttr);
  std::string real_path(filename);

  if (!src.empty()) {
    if (file_manager->GetFileContents(src.c_str(), &script, &real_path)) {
      filename = src.c_str();
      lineno = 1;
    }
  } else {
    // Uses the Windows version convention, that inline scripts should be
    // quoted in comments.
    const DOMNodeListInterface *children = xml_element->GetChildNodes();
    size_t length = children->GetLength();
    for (size_t i = 0; i < length; i++) {
      const DOMNodeInterface *child = children->GetItem(i);
      if (child->GetNodeType() == DOMNodeInterface::COMMENT_NODE) {
        script = child->GetTextContent();
        break;
      } else if (child->GetNodeType() != DOMNodeInterface::TEXT_NODE ||
                 !TrimString(child->GetTextContent()).empty()) {
        // Other contents are not allowed under <script></script>.
        LOG("%s:%d:%d: This content is not allowed in script element",
            filename, child->GetRow(), child->GetColumn());
      }
    }
    delete children;
  }

  if (!script.empty())
    script_context->Execute(script.c_str(), filename, lineno);
}

static void HandleAllScriptElements(ViewInterface *view,
                                    const char *filename,
                                    const DOMElementInterface *xml_element) {
  const DOMNodeListInterface *children = xml_element->GetChildNodes();
  size_t length = children->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMNodeInterface *child = children->GetItem(i);
    if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
      const DOMElementInterface *child_ele =
          down_cast<const DOMElementInterface *>(child);
      if (GadgetStrCmp(child_ele->GetTagName().c_str(), kScriptTag) == 0) {
        HandleScriptElement(view->GetScriptContext(),
                            view->GetFileManager(),
                            filename, child_ele);
      } else {
        HandleAllScriptElements(view, filename, child_ele);
      }
    }
  }
  delete children;
}

static BasicElement *InsertElementFromDOM(
    ViewInterface *view, Elements *elements,
    const char *filename, const DOMElementInterface *xml_element,
    const BasicElement *before) {
  std::string tag_name = xml_element->GetTagName();
  if (GadgetStrCmp(tag_name.c_str(), kScriptTag) == 0)
    return NULL;

  std::string name = xml_element->GetAttribute(kNameAttr);
  BasicElement *element = elements->InsertElement(tag_name.c_str(),
                                                      before,
                                                      name.c_str());
  if (!element) {
    LOG("%s:%d:%d: Failed to create element %s", filename,
        xml_element->GetRow(), xml_element->GetColumn(), tag_name.c_str());
    return element;
  }

  SetupScriptableProperties(element, view->GetScriptContext(),
                            filename, xml_element);
  Elements *children = element->GetChildren();
  const DOMNodeListInterface *xml_children = xml_element->GetChildNodes();
  std::string text;
  size_t length = xml_children->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMNodeInterface *child = xml_children->GetItem(i);
    DOMNodeInterface::NodeType type = child->GetNodeType();
    if (type == DOMNodeInterface::ELEMENT_NODE) {
      InsertElementFromDOM(view, children, filename,
                           down_cast<const DOMElementInterface *>(child),
                           NULL);
    } else if (type == DOMNodeInterface::TEXT_NODE ||
               type == DOMNodeInterface::CDATA_SECTION_NODE) {
      text += down_cast<const DOMTextInterface *>(child)->GetTextContent();
    }
  }
  // Set the "innerText" property.
  text = TrimString(text);
  if (!text.empty()) {
    SetScriptableProperty(element, view->GetScriptContext(), filename,
                          xml_element->GetRow(), xml_element->GetColumn(),
                          kInnerTextProperty, text.c_str(), tag_name.c_str());
  }
  delete xml_children;
  return element;
}

bool SetupViewFromFile(ViewInterface *view, const char *filename) {
  std::string contents;
  std::string real_path;
  if (!view->GetFileManager()->GetXMLFileContents(filename,
                                                  &contents, &real_path))
    return false;

  return SetupViewFromXML(view, contents, real_path.c_str());
}

bool SetupViewFromXML(ViewInterface *view, const std::string &xml,
                      const char *filename) {
  DOMDocumentInterface *xmldoc = view->GetXMLParser()->CreateDOMDocument();
  xmldoc->Attach();
  if (!view->GetXMLParser()->ParseContentIntoDOM(xml, filename, NULL, NULL,
                                                 xmldoc, NULL, NULL)) {
    xmldoc->Detach();
    return false;
  }

  DOMElementInterface *view_element = xmldoc->GetDocumentElement();
  if (!view_element ||
      GadgetStrCmp(view_element->GetTagName().c_str(), kViewTag) != 0) {
    LOG("No valid root element in view file: %s", filename);
    xmldoc->Detach();
    return false;
  }

  SetupScriptableProperties(view, view->GetScriptContext(),
                            filename, view_element);

  Elements *children = view->GetChildren();
  const DOMNodeListInterface *xml_children = view_element->GetChildNodes();
  size_t length = xml_children->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMNodeInterface *child = xml_children->GetItem(i);
    if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
      InsertElementFromDOM(view, children, filename,
                           down_cast<const DOMElementInterface *>(child),
                           NULL);
    }
  }

  HandleAllScriptElements(view, filename, view_element);
  delete xml_children;
  xmldoc->Detach();
  return true;
}

BasicElement *AppendElementFromXML(ViewInterface *view,
                                       Elements *elements,
                                       const std::string &xml) {
  return InsertElementFromXML(view, elements, xml, NULL);
}

BasicElement *InsertElementFromXML(ViewInterface *view,
                                       Elements *elements,
                                       const std::string &xml,
                                       const BasicElement *before) {
  DOMDocumentInterface *xmldoc = view->GetXMLParser()->CreateDOMDocument();
  xmldoc->Attach();
  if (!view->GetXMLParser()->ParseContentIntoDOM(xml, xml.c_str(), NULL, NULL,
                                                 xmldoc, NULL, NULL)) {
    xmldoc->Detach();
    return false;
  }

  DOMElementInterface *xml_element = xmldoc->GetDocumentElement();
  if (!xml_element) {
    LOG("No root element in xml definition: %s", xml.c_str());
    xmldoc->Detach();
    return NULL;
  }

  BasicElement *result = InsertElementFromDOM(view, elements, "",
                                              xml_element, before);
  xmldoc->Detach();
  return result;
}

} // namespace ggadget
