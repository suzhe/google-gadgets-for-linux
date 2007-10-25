/*
  Copyright 2007 Google Inc.

  virtual Licensed under the Apache License, Version 2.0 (the "License") = 0;
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_XML_DOM_INTERFACE_H__
#define GGADGET_XML_DOM_INTERFACE_H__

#include "common.h"
#include "scriptable_interface.h"
#include "unicode_utils.h"

namespace ggadget {

class DOMNodeListInterface;
class DOMNamedNodeMapInterface;
class DOMDocumentInterface;
class DOMElementInterface;

/* TODO: DOM2
const char *const kXMLPrefix = "xml";
const char *const kXMLNamespaceURI = "http://www.w3.org/XML/1998/namespace";
const char *const kXMLNSPrefix = "xmlns";
const char *const kXMLNSNamespaceURI = "http://www.w3.org/2000/xmlns/";
*/

const char *const kCDATASectionName = "#cdata-section";
const char *const kCommentName = "#comment";
const char *const kDocumentName = "#document";
const char *const kDocumentFragmentName = "#document-fragment";
const char *const kTextName = "#text";

/**
 * DOM interfaces.
 * Reference:
 *   - http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113/
 */
enum DOMExceptionCode {
  /**
   * @c NO_ERR is only used in C++ API as return values of methods to
   * indicate no error occurs. It should not be reflected to scripts.
   */
  DOM_NO_ERR,
  DOM_INDEX_SIZE_ERR              = 1,
  DOM_DOMSTRING_SIZE_ERR          = 2,
  DOM_HIERARCHY_REQUEST_ERR       = 3,
  DOM_WRONG_DOCUMENT_ERR          = 4,
  DOM_INVALID_CHARACTER_ERR       = 5,
  DOM_NO_DATA_ALLOWED_ERR         = 6,
  DOM_NO_MODIFICATION_ALLOWED_ERR = 7,
  DOM_NOT_FOUND_ERR               = 8,
  DOM_NOT_SUPPORTED_ERR           = 9,
  DOM_INUSE_ATTRIBUTE_ERR         = 10,
  /* TODO: DOM2
  DOM_INVALID_STATE_ERR           = 11,
  DOM_SYNTAX_ERR                  = 12,
  DOM_INVALID_MODIFICATION_ERR    = 13,
  DOM_NAMESPACE_ERR               = 14,
  DOM_INVALID_ACCESS_ERR          = 15,
  */
  /** Extended error code to indicate an unexpected null pointer argument. */
  DOM_NULL_POINTER_ERR            = 100,
};

class DOMNodeInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x7787eb3be55b4266);

  enum NodeType {
    ELEMENT_NODE                = 1,
    ATTRIBUTE_NODE              = 2,
    TEXT_NODE                   = 3,
    CDATA_SECTION_NODE          = 4,
    ENTITY_REFERENCE_NODE       = 5,
    ENTITY_NODE                 = 6,
    PROCESSING_INSTRUCTION_NODE = 7,
    COMMENT_NODE                = 8,
    DOCUMENT_NODE               = 9,
    DOCUMENT_TYPE_NODE          = 10,
    DOCUMENT_FRAGMENT_NODE      = 11,
    NOTATION_NODE               = 12,
  };

  /** For implementation only. */
  virtual void *GetImplData() = 0;

  virtual const char *GetNodeName() const = 0;
  virtual const char *GetNodeValue() const = 0;
  virtual void SetNodeValue(const char *node_value) = 0;
  virtual NodeType GetNodeType() const = 0;

  virtual DOMNodeInterface *GetParent() = 0;
  virtual const DOMNodeInterface *GetParentNode() const = 0;
  virtual DOMNodeListInterface *GetChildNodes() = 0;
  virtual const DOMNodeListInterface *GetChildNodes() const = 0;
  virtual DOMNodeInterface *GetFirstChild() = 0;
  virtual const DOMNodeInterface *GetFirstChild() const = 0;
  virtual DOMNodeInterface *GetLastChild() = 0;
  virtual const DOMNodeInterface *GetLastChild() const = 0;
  virtual DOMNodeInterface *GetPreviousSibling() = 0;
  virtual const DOMNodeInterface *GetPreviousSibling() const = 0;
  virtual DOMNodeInterface *GetNextSibling() = 0;
  virtual const DOMNodeInterface *GetNextSibling() const = 0;
  virtual DOMNamedNodeMapInterface *GetAttributes() = 0;
  virtual const DOMNamedNodeMapInterface *GetAttributes() const = 0;
  virtual DOMDocumentInterface *GetOwnerDocument() = 0;
  virtual const DOMDocumentInterface *GetOwnerDocument() const = 0;

  virtual DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                        DOMNodeInterface *ref_child) = 0;
  virtual DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                        DOMNodeInterface *old_child) = 0;
  virtual DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) = 0;
  virtual DOMExceptionCode AppendChild(DOMNodeInterface *new_child) = 0;

  virtual bool HasChildNodes() const = 0;
  virtual DOMNodeInterface *CloneNode(bool deep) = 0;

  /**
   * Though Node.normalize() is in DOM2, DOM1 has only Element.normalize().
   * Declare it here for convenience.
   * We can prevent script program from accesssing it by only registering the
   * script method into Element class only.
   */
  virtual void Normalize() = 0;

  /**
   * Declare these methods here for convenience.
   * We can prevent script program from accesssing it by only registering the
   * script method into Element and Document classes only.
   */
  virtual DOMNodeListInterface *GetElementsByTagName(const char *name) = 0;
  virtual const DOMNodeListInterface *GetElementsByTagName(
      const char *name) const = 0;

  /* TODO: DOM2
  virtual bool IsSupported(const char *feature, const char *version) const = 0;
  virtual const char *GetNamespaceURI() const = 0;
  virtual const char *GetPrefix() const = 0;
  virtual DOMExceptionCode SetPrefix(const char *prefix) = 0;
  virtual const char *GetLocalName() const = 0;
  virtual bool HasAttributes() const = 0;
  // Declare these methods here for convenience.
  virtual DOMNodeListInterface *GetElementsByTagNameNS(
      const char *namespace_uri, const char *local_name) = 0;
  virtual const DOMNodeListInterface *GetElementsByTagNameNS(
      const char *namespace_uri, const char *local_name) const = 0;
  */
};
CLASS_ID_IMPL(DOMNodeInterface, ScriptableInterface)

class DOMNodeListInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x9935a8188f734afe);

  virtual DOMNodeInterface *GetItem(size_t index) = 0;
  virtual const DOMNodeInterface *GetItem(size_t index) const = 0;
  virtual size_t GetLength() const = 0;
};
CLASS_ID_IMPL(DOMNodeListInterface, ScriptableInterface)

class DOMNamedNodeMapInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0xd2c849db6fb6416f);

  virtual DOMNodeInterface *GetNamedItem(const char *name) = 0;
  virtual const DOMNodeInterface *GetNamedItem(const char *name) const = 0;
  virtual DOMExceptionCode SetNamedItem(DOMNodeInterface *arg) = 0;
  virtual DOMExceptionCode RemoveNamedItem(
      const char *name, DOMNodeInterface **removed_node) = 0;
  virtual DOMNodeInterface *GetItem(size_t index) = 0;
  virtual const DOMNodeInterface *GetItem(size_t index) const = 0;
  virtual size_t GetLength() const = 0;

  /* TODO: DOM2
  virtual DOMNodeInterface *GetNamedItemNS(const char *namespace_uri,
                                   const char *local_name) = 0;
  virtual const DOMNodeInterface *GetNamedItemNS(const char *namespace_uri,
                                         const char *local_name) const = 0;
  virtual DOMExceptionCode SetNamedItemNS(DOMNodeInterface *arg) = 0;
  virtual DOMExceptionCode RemoveNamedItemNS(
      const char *namespace_uri,
      const char *local_name,
      DOMNodeInterface **removed_node) = 0;
  */
};
CLASS_ID_IMPL(DOMNamedNodeMapInterface, ScriptableInterface)

class DOMCharacterDataInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x199ea7a610e048b9);

  virtual const UTF16Char *GetData() const = 0;
  virtual void SetData(const UTF16Char *data) = 0;
  virtual size_t GetLength() const = 0;
  /** The caller should free the pointer with delete [] operator after use. */
  virtual DOMExceptionCode SubstringData(size_t offset, size_t count,
                                         UTF16Char **result) const = 0;
  virtual void AppendData(const UTF16Char *arg) = 0;
  virtual DOMExceptionCode InsertData(size_t offset, const UTF16Char *arg) = 0;
  virtual DOMExceptionCode DeleteData(size_t offset, size_t count) = 0;
  virtual DOMExceptionCode ReplaceData(size_t offset, size_t count,
                                       const UTF16Char *arg) = 0;
};
CLASS_ID_IMPL(DOMCharacterDataInterface, DOMNodeInterface)

class DOMAttrInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0xc1c04a2ea6ed45fc);

  virtual const char *GetName() const = 0;
  virtual bool IsSpecified() const = 0;
  virtual const char *GetValue() const = 0;
  virtual void SetValue(const char *value) = 0;

  /** DOM2 property, but useful. */
  virtual DOMElementInterface *GetOwnerElement() = 0;
  virtual const DOMElementInterface *GetOwnerElement() const = 0;
};
CLASS_ID_IMPL(DOMAttrInterface, DOMNodeInterface)

class DOMElementInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x98722c98a65a4801);

  virtual const char *GetTagName() const = 0;
  virtual const char *GetAttribute(const char *name) const = 0;
  virtual DOMExceptionCode SetAttribute(const char *name,
                                        const char *value) = 0;
  virtual void RemoveAttribute(const char *name) = 0;
  virtual DOMAttrInterface *GetAttributeNode(const char *name) = 0;
  virtual const DOMAttrInterface *GetAttributeNode(const char *name) const = 0;
  virtual DOMExceptionCode SetAttributeNode(
      DOMAttrInterface *new_attr, DOMAttrInterface **replaced_attr) = 0;
  virtual DOMExceptionCode RemoveAttributeNode(DOMAttrInterface *old_attr) = 0;
  // GetElementsByTagName has been declared in DOMNodeInterface.
  virtual DOMNamedNodeMapInterface *GetAttributes() = 0;
  virtual const DOMNamedNodeMapInterface *GetAttributes() const = 0;

  /* TODO: DOM2
  virtual const char *GetAttributeNS(const char *namespace_uri,
                                     const char *local_name) = 0;
  virtual DOMExceptionCode SetAttributeNS(const char *namespace_uri,
                                          const char *qualified_name,
                                          const char *value) = 0;
  virtual DOMExceptionCode RemoveAttributeNS(const char *namespace_uri,
                                             const char *local_name) = 0;
  virtual DOMAttrInterface *GetAttributeNodeNS(const char *namespace_uri,
                                               const char *local_name) = 0;
  virtual const DOMAttrInterface *GetAttributeNodeNS(
      const char *namespace_uri, const char *local_name) const = 0;
  virtual DOMExceptionCode SetAttributeNodeNS(
      DOMAttrInterface *new_attr, DOMAttrInterface **replaced_attr) = 0;
  // GetElementsByTagNameNS has been declared in DOMNodeInterface.
  virtual bool HasAttribute(const char *name) const = 0;
  virtual bool HasAttributeNS(const char *namespace_uri,
                              const char *local_name) const = 0;
  */
};
CLASS_ID_IMPL(DOMElementInterface, DOMNodeInterface)

class DOMTextInterface : public DOMCharacterDataInterface {
 public:
  CLASS_ID_DECL(0x401b780c290c4525);
  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) = 0;
};
CLASS_ID_IMPL(DOMTextInterface, DOMCharacterDataInterface)

class DOMCDATASectionInterface : public DOMTextInterface {
 public:
  CLASS_ID_DECL(0x16ce6e727f694f7b);
};
CLASS_ID_IMPL(DOMCDATASectionInterface, DOMTextInterface)

class DOMDocumentFragmentInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x349f983c7e1c4407);
};
CLASS_ID_IMPL(DOMDocumentFragmentInterface, DOMNodeInterface)

class DOMDocumentTypeInterface;
class DOMProcesssingInstructionInterface;
class DOMEntityReferenceInterface;

class DOMImplementationInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x92586d525bf34b13);

  virtual bool HasFeature(const char *feature, const char *version) const = 0;

  /* TODO: DOM2
  virtual DOMExceptionCode CreateDocumentType(
      const char *qualified_name,
      const char *public_id, const char *system_id,
      DOMDocumentTypeInterface **result) = 0;
  virtual DOMExceptionCode CreateDocument(
      const char *namespace_uri, const char *qualified_name,
      const DOMDocumentTypeInterface *doctype,
      DOMDocumentInterface **result) = 0;
  */
};
CLASS_ID_IMPL(DOMImplementationInterface, ScriptableInterface)

class DOMDocumentInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x885f4371c0024a79);

  virtual DOMDocumentTypeInterface *GetDoctype() = 0;
  virtual const DOMDocumentTypeInterface *GetDoctype() const = 0;
  virtual DOMImplementationInterface *GetImplementation() = 0;
  virtual const DOMImplementationInterface *GetImplementation() const = 0;
  virtual DOMElementInterface *GetDocumentElement() = 0;
  virtual const DOMElementInterface *GetDocumentElement() const = 0;
  virtual DOMExceptionCode CreateElement(const char *tag_name,
                                         DOMElementInterface **result) = 0;
  virtual DOMDocumentFragmentInterface *CreateDocumentFragment() = 0;
  virtual DOMTextInterface *CreateTextNode(const UTF16Char *data) = 0;
  virtual DOMCDATASectionInterface *CreateCDATASection(
      const UTF16Char *data) = 0;
  virtual DOMExceptionCode CreateProcessingInstruction(
      const char *target, const char *data,
      DOMProcesssingInstructionInterface **result) = 0;
  virtual DOMExceptionCode CreateAttribute(const char *name,
                                           DOMAttrInterface **result) = 0;
  virtual DOMExceptionCode CreateEntityReference(
      const char *name, DOMEntityReferenceInterface **result) = 0;
  // GetElementsByName is declared in DOMNodeInterface.

  /* TODO: DOM2
  virtual DOMExceptionCode ImportNode(DOMNodeInterface *imported_node,
                                      bool deep) = 0;
  virtual DOMExceptionCode CreateElementNS(const char *namespace_uri,
                                           const char *qualified_name,
                                           DOMElementInterface **result) = 0;
  virtual DOMExceptionCode CreateAttributeNS(const char *namespace_uri,
                                             const char *qualified_name,
                                             DOMAttrInterface **result) = 0;
  // GetElementsByNameNS is declared in DOMNodeInterface.
  virtual DOMElementInterface *GetElementById(const char *element_id) = 0;
  virtual const DOMElementInterface *GetElementById(
      const char *element_id) const = 0;
  */
};
CLASS_ID_IMPL(DOMDocumentInterface, DOMNodeInterface)

} // namespace ggadget

#endif // GGADGET_XML_DOM_INTERFACE_H__
