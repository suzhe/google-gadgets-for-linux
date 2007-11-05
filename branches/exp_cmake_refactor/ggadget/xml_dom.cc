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

#include <algorithm>
#include <vector>
#include "xml_dom.h"
#include "scriptable_helper.h"
#include "xml_utils.h"

namespace ggadget {
namespace internal {

// Constants for XML pretty printing.
static const size_t kLineLengthThreshold = 70;
static const size_t kIndent = 1;
static const char *kStandardXMLDecl =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

static const char *kExceptionNames[] = {
    "",
    "INDEX_SIZE_ERR",
    "DOMSTRING_SIZE_ERR",
    "HIERARCHY_REQUEST_ERR",
    "WRONG_DOCUMENT_ERR",
    "INVALID_CHARACTER_ERR",
    "NO_DATA_ALLOWED_ERR",
    "NO_MODIFICATION_ALLOWED_ERR",
    "NOT_FOUND_ERR",
    "NOT_SUPPORTED_ERR",
    "INUSE_ATTRIBUTE_ERR",
};

class GlobalException : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x81f363ca1c034f39, ScriptableInterface);
  static GlobalException *Get() {
    static internal::GlobalException global_exception;
    return &global_exception;
  }
 private:
  GlobalException() {
    RegisterConstants(arraysize(kExceptionNames), kExceptionNames, NULL);
  }
};

static const char *kNodeTypeNames[] = {
    ""
    "ELEMENT_NODE",
    "ATTRIBUTE_NODE",
    "TEXT_NODE",
    "CDATA_SECTION_NODE",
    "ENTITY_REFERENCE_NODE",
    "ENTITY_NODE",
    "PROCESSING_INSTRUCTION_NODE",
    "COMMENT_NODE",
    "DOCUMENT_NODE",
    "DOCUMENT_TYPE_NODE",
    "DOCUMENT_FRAGMENT_NODE",
    "NOTATION_NODE",
};

class GlobalNode : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x2a9d299fb51c4070, ScriptableInterface);
  static GlobalNode *Get() {
    static internal::GlobalNode global_node;
    return &global_node;
  }
 private:
  GlobalNode() {
    RegisterConstants(arraysize(kNodeTypeNames), kNodeTypeNames, NULL);
  }
};

class DOMException : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x6486921444b44784, ScriptableInterface);

  DOMException(DOMExceptionCode code) : code_(code) {
    RegisterSimpleProperty("code", &code_);
    SetPrototype(GlobalException::Get());
  }

  // This is a script owned object.
  virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; } 
  virtual bool Detach() { delete this; return true; }

 private:
  DOMExceptionCode code_;
};

// Used in the methods for script to throw an script exception on errors.
static void CheckException(DOMExceptionCode code)
    throw(ScriptableExceptionHolder) {
  if (code != DOM_NO_ERR) {
    DLOG("Throw DOMException: %d", code);
    throw ScriptableExceptionHolder(new DOMException(code));
  }
}

// Check if child type is acceptable for Element, DocumentFragment,
// EntityReference and Entity nodes.
static DOMExceptionCode CheckCommonChildType(DOMNodeInterface *new_child) {
  DOMNodeInterface::NodeType type = new_child->GetNodeType();
  if (type != DOMNodeInterface::ELEMENT_NODE &&
      type != DOMNodeInterface::TEXT_NODE &&
      type != DOMNodeInterface::COMMENT_NODE &&
      type != DOMNodeInterface::PROCESSING_INSTRUCTION_NODE &&
      type != DOMNodeInterface::CDATA_SECTION_NODE &&
      type != DOMNodeInterface::ENTITY_REFERENCE_NODE)
    return DOM_HIERARCHY_REQUEST_ERR;
  return DOM_NO_ERR;
}

class DOMNodeListBase : public ScriptableHelper<DOMNodeListInterface> {
 public:
  using DOMNodeListInterface::GetItem;
  DOMNodeListBase() {
    DOMNodeListInterface *super_ptr =
        implicit_cast<DOMNodeListInterface *>(this);
    RegisterProperty("length",
                     NewSlot(super_ptr, &DOMNodeListInterface::GetLength),
                     NULL);
    RegisterMethod("item", NewSlot(super_ptr, &DOMNodeListInterface::GetItem));
    SetArrayHandler(NewSlot(super_ptr, &DOMNodeListInterface::GetItem), NULL);
  }
};

// The DOMNodeList used as the return value of GetElementsByTagName().
class ElementsByTagName : public DOMNodeListBase {
 public:
  DEFINE_CLASS_ID(0x08b36d84ae044941, DOMNodeListInterface);

  ElementsByTagName(DOMNodeInterface *node, const char *name)
      : node_(node),
        name_(name ? name : ""),
        wildcard_(name && name[0] == '*' && name[1] == '\0') {
  }

  // ElementsByTagName is not reference counted. The Attach() and Detach()
  // methods are only for the script adapter.
  virtual OwnershipPolicy Attach() {
    node_->Attach();
    return OWNERSHIP_TRANSFERRABLE;
  }
  virtual bool Detach() {
    node_->Detach();
    delete this;
    return true;
  }

  virtual DOMNodeInterface *GetItem(size_t index) {
    return const_cast<DOMNodeInterface *>(GetItemFromNode(node_, &index));
  }
  virtual const DOMNodeInterface *GetItem(size_t index) const {
    return GetItemFromNode(node_, &index);
  }

  virtual size_t GetLength() const {
    return CountChildElements(node_);
  }

 private:
  const DOMNodeInterface *GetItemFromNode(const DOMNodeInterface *node,
                                          size_t *index) const {
    const DOMNodeListInterface *children = node->GetChildNodes();
    const DOMNodeInterface *result_item = NULL;
    size_t length = children->GetLength();
    for (size_t i = 0; i < length; i++) {
      const DOMNodeInterface *item = children->GetItem(i);
      if (item->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
        // This node first and then children.
        if (wildcard_ || name_ == item->GetNodeName()) {
          if (*index == 0) {
            result_item = item;
            break;
          }
          (*index)--;
        }

        const DOMNodeInterface *result = GetItemFromNode(item, index);
        if (result) {
          // Found in children.
          ASSERT(*index == 0);
          result_item = result;
          break;
        }
      }
    }

    delete children;
    return result_item;
  }

  size_t CountChildElements(const DOMNodeInterface *node) const {
    const DOMNodeListInterface *children = node->GetChildNodes();
    size_t length = children->GetLength();
    size_t count = 0;
    for (size_t i = 0; i < length; i++) {
      const DOMNodeInterface *item = children->GetItem(i);
      if (item->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
        if (wildcard_ || name_ == item->GetNodeName())
          count++;
        count += CountChildElements(item);
      }
    }
    delete children;
    return count;
  }

  DOMNodeInterface *node_;
  std::string name_;
  bool wildcard_;
};

// Append a '\n' and indent to xml.
static void AppendIndentNewLine(size_t indent, std::string *xml) {
  if (!xml->empty() && *(xml->end() - 1) != '\n')
    xml->append(1, '\n');
  xml->append(indent, ' ');
}

// Append indent if the current position is a new line.
static void AppendIndentIfNewLine(size_t indent, std::string *xml) {
  if (xml->empty() || *(xml->end() - 1) == '\n')
    xml->append(indent, ' ');
}

// Interface for DOMNodeImpl callbacks to its node.
class DOMNodeImplCallbacks {
 public:
  virtual ~DOMNodeImplCallbacks() { }
  // Subclasses should implement these methods.
  virtual DOMNodeInterface *CloneSelf() = 0;
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) = 0;
  // Append the XML string representation to the string.
  virtual void AppendXML(size_t indent, std::string *xml) = 0;
};

class DOMNodeImpl {
 public:
  // Using vector is simple to implement. Since most elements have not
  // many children, the performance degration is trivial.
  typedef std::vector<DOMNodeInterface *> Children;

  DOMNodeImpl(DOMNodeInterface *node,
              DOMNodeImplCallbacks *callbacks,
              DOMDocumentInterface *owner_document,
              const char *name)
      : node_(node),
        callbacks_(callbacks),
        owner_document_(owner_document),
        name_(name ? name : ""),
        parent_(NULL),
        owner_node_(NULL),
        ref_count_(0) {
    ASSERT(name && *name);
    if (name != kDOMDocumentName) {
      ASSERT(owner_document_);
      // Any newly created node has no parent and thus is orphan. Increase the
      // document orphan count.
      owner_document_->Attach();
    }
  }

  virtual ~DOMNodeImpl() {
    ASSERT(ref_count_ == 0);

    if (!owner_node_ && owner_document_) {
      // The node is still an orphan. Remove the document orphan count.
      owner_document_->Detach();
    }

    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it)
      delete *it;
    children_.clear();
  }

  // Called when a tree is appended to this node or one of its descendants.
  void AttachMulti(int count) {
    ASSERT(ref_count_ >= 0 && count >= 0);
    if (count > 0) {
      ref_count_ += count;
      if (owner_node_) {
        // Increase the reference count along the path to the root.
        owner_node_->GetImpl()->AttachMulti(count);
      }
    }
  }

  // Called when a tree is removed from this node or one of its descendants.
  // If transient is true, the node will not be deleted even if the reference
  // count reaches zero. This is useful to return nodes from methods like
  // RemoveXXX() and ReplaceXXX().
  bool DetachMulti(int count, bool transient) {
    ASSERT(ref_count_ >= count && count >= 0);
    if (count > 0) {
      ref_count_ -= count;
      if (owner_node_) {
        // Decrease the reference count along the path to the root.
        owner_node_->GetImpl()->DetachMulti(count, transient);
      } else if (ref_count_ == 0 && !transient) {
        // Only the root can delete the whole tree. Because the reference count
        // are accumulated, root's ref_count_ == 0 means all decendents's
        // ref_count_ == 0, thus the whole tree can be safely deleted.
        delete node_;
        return true;
      }
    }
    return false;
  }

  DOMNodeListInterface *GetChildNodes() {
    return new ChildrenNodeList(node_, children_);
  }
  DOMNodeInterface *GetFirstChild() {
    return children_.empty() ? NULL : children_.front();
  }
  DOMNodeInterface *GetLastChild() {
    return children_.empty() ? NULL : children_.back();
  }

  DOMNodeInterface *GetPreviousSibling() {
    if (!parent_)
      return NULL;

    DOMNodeImpl *parent_impl = parent_->GetImpl();
    Children::iterator it = parent_impl->FindChild(node_);
    return it == parent_impl->children_.begin() ? NULL : *(it - 1);
  }
  DOMNodeInterface *GetNextSibling() {
    if (!parent_)
      return NULL;

    DOMNodeImpl *parent_impl = parent_->GetImpl();
    Children::iterator it = parent_impl->FindChild(node_) + 1;
    return it == parent_impl->children_.end() ? NULL : *it;
  }

  DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                DOMNodeInterface *ref_child) {
    if (!new_child)
      return DOM_NULL_POINTER_ERR;

    if (ref_child && ref_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;

    if (new_child->GetNodeType() == DOMNodeInterface::DOCUMENT_FRAGMENT_NODE) {
      DOMNodeListInterface *children = new_child->GetChildNodes();
      DOMExceptionCode code = DOM_NO_ERR;
      while (children->GetLength() > 0) {
        code = InsertBefore(children->GetItem(0), ref_child);
        if (code != DOM_NO_ERR)
          break;
      }
      delete children;
      return code;
    }

    DOMExceptionCode code = callbacks_->CheckNewChild(new_child);
    if (code != DOM_NO_ERR)
      return code;

    if (new_child == ref_child)
      return DOM_NO_ERR;

    // Remove the new_child from its old parent.
    DOMNodeInterface *old_parent = new_child->GetParentNode();
    if (old_parent) {
      DOMNodeImpl *old_parent_impl = old_parent->GetImpl();
      old_parent_impl->children_.erase(old_parent_impl->FindChild(new_child));
      // old_parent's reference will be updated with new_child->SetParent().
    }

    Children::iterator it = ref_child ? FindChild(ref_child) : children_.end();
    children_.insert(it, new_child);

    new_child->GetImpl()->SetParent(node_);
    return DOM_NO_ERR;
  }

  DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                DOMNodeInterface *old_child) {
    if (!new_child || !old_child)
      return DOM_NULL_POINTER_ERR;
    if (old_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;
    if (new_child == old_child)
      return DOM_NO_ERR;

    DOMExceptionCode code = InsertBefore(new_child, old_child);
    if (code != DOM_NO_ERR)
      return code;

    return RemoveChild(old_child);
  }

  DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) {
    if (!old_child)
      return DOM_NULL_POINTER_ERR;
    if (old_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;

    children_.erase(FindChild(old_child));
    old_child->GetImpl()->SetParent(NULL);
    return DOM_NO_ERR;
  }

  DOMNodeInterface *CloneNode(bool deep) {
    DOMNodeInterface *self_cloned = callbacks_->CloneSelf();
    if (self_cloned && deep) {
      for (Children::iterator it = children_.begin();
           it != children_.end();
           ++it)
        // Ignore error returned from AppendChild, since it should not occur.
        self_cloned->AppendChild((*it)->CloneNode(deep));
    }
    return self_cloned;
  }

  void Normalize() {
    for (size_t i = 0; i < children_.size(); i++) {
      DOMNodeInterface *child = children_[i];
      if (child->GetNodeType() == DOMNodeInterface::TEXT_NODE) {
        DOMTextInterface *text = down_cast<DOMTextInterface *>(child);
        if (text->GetData()[0] == 0) {
          // Remove empty text nodes.
          RemoveChild(text);
          i--;
        } else if (i > 0) {
          DOMNodeInterface *last_child = children_[i - 1];
          if (last_child->GetNodeType() == DOMNodeInterface::TEXT_NODE) {
            // Merge the two node into one.
            DOMTextInterface *text0 = down_cast<DOMTextInterface *>(last_child);
            text0->InsertData(text0->GetLength(), text->GetData());
            RemoveChild(text);
            i--;
          }
        }
      } else {
        child->Normalize();
      }
    }
  }

  const char *GetChildrenTextContent() {
    last_children_text_content_.clear();
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      DOMNodeInterface::NodeType type = (*it)->GetNodeType();
      if (type != DOMNodeInterface::COMMENT_NODE &&
          type != DOMNodeInterface::PROCESSING_INSTRUCTION_NODE)
        last_children_text_content_ += (*it)->GetTextContent();
    }
    return last_children_text_content_.c_str();
  }

  void SetChildTextContent(const char *text_content) {
    RemoveAllChildren();
    UTF16String utf16_content;
    if (text_content) {
      ConvertStringUTF8ToUTF16(text_content, strlen(text_content),
                               &utf16_content);
    }
    InsertBefore(owner_document_->CreateTextNode(utf16_content.c_str()), NULL);
  }

  const char *GetXML() {
    last_xml_.clear();
    callbacks_->AppendXML(0, &last_xml_);
    return last_xml_.c_str();
  }

 public:
  // The following public methods are utilities for interface implementations.
  void AppendChildrenXML(size_t indent, std::string *xml) {
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      (*it)->GetImpl()->callbacks_->AppendXML(indent, xml);
    }
  }

  void RemoveAllChildren() {
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) 
      (*it)->GetImpl()->SetParent(NULL);
    children_.clear();
  }

  DOMExceptionCode CheckNewChildCommon(DOMNodeInterface *new_child) {
    // The new_child must be in the same document of this node.
    DOMDocumentInterface *new_child_doc = new_child->GetOwnerDocument();
    if ((owner_document_ && new_child_doc != owner_document_) ||
        // In case of the current node is the document.
        (!owner_document_ && new_child_doc != node_)) {
      DLOG("CheckNewChildCommon: Wrong document");
      return DOM_WRONG_DOCUMENT_ERR;
    }

    // The new_child can't be this node itself or one of this node's ancestors.
    for (DOMNodeInterface *ancestor = node_; ancestor != NULL;
         ancestor = ancestor->GetParentNode()) {
      if (ancestor == new_child) {
        DLOG("CheckNewChildCommon: New child is self or ancestor");
        return DOM_HIERARCHY_REQUEST_ERR;
      }
    }

    return DOM_NO_ERR;
  }

  Children::iterator FindChild(DOMNodeInterface *child) {
    ASSERT(child && child->GetParentNode() == node_);
    Children::iterator it = std::find(children_.begin(),
                                      children_.end(), child);
    ASSERT(it != children_.end());
    return it;
  }

  DOMNodeInterface *ScriptInsertBefore(DOMNodeInterface *new_child,
                                       DOMNodeInterface *ref_child) {
    CheckException(InsertBefore(new_child, ref_child));
    return new_child;
  }

  DOMNodeInterface *ScriptReplaceChild(DOMNodeInterface *new_child,
                                       DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Attach();
    DOMExceptionCode code = ReplaceChild(new_child, old_child);
    // Remove the transient reference but still keep the node if there is no
    // error.
    if (old_child)
      old_child->GetImpl()->DetachMulti(1, code == DOM_NO_ERR);
    CheckException(code);
    return old_child;
  }

  DOMNodeInterface *ScriptRemoveChild(DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Attach();
    DOMExceptionCode code = RemoveChild(old_child);
    // Remove the transient reference but still keep the node.
    if (old_child)
      old_child->GetImpl()->DetachMulti(1, code == DOM_NO_ERR);
    CheckException(code);
    return old_child;
  }

  DOMNodeInterface *ScriptAppendChild(DOMNodeInterface *new_child) {
    return ScriptInsertBefore(new_child, NULL);
  }

  void SetParent(DOMNodeInterface *new_parent) {
    parent_ = new_parent;
    SetOwnerNode(new_parent);
  }

  // The implementation classes must call this method when the owner node
  // changes. In most cases, the owner node is the parent node, but DOMAttr
  // is an exception, whose owner node is the owner element.
  void SetOwnerNode(DOMNodeInterface *new_owner) {
    if (owner_node_ != new_owner) {
      if (owner_node_) {
        // This node is detached from the old owner node.
        owner_node_->GetImpl()->DetachMulti(ref_count_, false);
        if (!new_owner) {
          // This node becomes a new orphan.
          if (ref_count_ == 0) {
            // This orphan is not referenced, delete it now.
            delete node_;
            return;
          } else {
            // This orphan is still referenced. Increase the document
            // orphan count.
            owner_document_->Attach();
          }
        }
      }

      if (new_owner) {
        // This node is attached to the new owner node.
        new_owner->GetImpl()->AttachMulti(ref_count_);
        if (!owner_node_) {
          // The node is not orphan now, so decrease the document orphan count.
          owner_document_->Detach();
        }
      }
      owner_node_ = new_owner;
    }
  }

  // The DOMNodeList used to enumerate this node's children.
  class ChildrenNodeList : public DOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0x72b1fc54e58041ae, DOMNodeListInterface);

    ChildrenNodeList(DOMNodeInterface *node,
                     const Children &children)
        : node_(node), children_(children) { }

    // References to this object are counted to the owner node.
    // ChildrenNodeList is not reference counted. The Attach() and Detach()
    // methods are only for the script adapter.
    virtual OwnershipPolicy Attach() {
      node_->Attach();
      return OWNERSHIP_TRANSFERRABLE;
    }
    virtual bool Detach() {
      node_->Detach();
      delete this;
      return true;
    }

    DOMNodeInterface *GetItem(size_t index) {
      return index < children_.size() ? children_[index] : NULL;
    }
    const DOMNodeInterface *GetItem(size_t index) const {
      return index < children_.size() ? children_[index] : NULL;
    }
    size_t GetLength() const { return children_.size(); }

   private:
    DOMNodeInterface *node_;
    const Children &children_;
  };

  // In fact, node_ and callbacks_ points to the same object.
  DOMNodeInterface *node_;
  DOMNodeImplCallbacks *callbacks_;
  DOMDocumentInterface *owner_document_;
  std::string name_;
  DOMNodeInterface *parent_;
  // In most cases, owner_node_ == parent_, but for DOMAttr, owner_node_ is the
  // owner element.
  DOMNodeInterface *owner_node_;
  Children children_;
  std::string last_children_text_content_;
  std::string last_xml_;

  // This ref_count_ records the accumulated reference count of all descendants.
  // ref_count_ == 0 means all descendants' ref_count == 0.
  // The references among the nodes in the DOM tree are not counted.
  int ref_count_;
};

template <typename Interface>
class DOMNodeBase : public ScriptableHelper<Interface>,
                    public DOMNodeImplCallbacks {
 public:
  // The following is to overcome template inheritance limits.
  typedef ScriptableHelper<Interface> Super;
  using Super::RegisterConstant;
  using Super::RegisterProperty;
  using Super::RegisterSimpleProperty;
  using Super::RegisterReadonlySimpleProperty;
  using Super::RegisterMethod;
  using Super::SetPrototype;
  using Super::SetDynamicPropertyHandler;
  using Super::SetArrayHandler;

  DOMNodeBase(DOMDocumentInterface *owner_document,
              const char *name)
      : impl_(new DOMNodeImpl(this, this, owner_document, name)) {
    RegisterConstant("nodeName", impl_->name_);
    RegisterProperty("nodeValue", NewSlot(this, &DOMNodeBase::GetNodeValue),
                     NewSlot(this, &DOMNodeBase::SetNodeValue));
    RegisterProperty("nodeType",
                     NewSlot(implicit_cast<DOMNodeInterface *>(this),
                             &DOMNodeInterface::GetNodeType),
                     NULL);
    RegisterReadonlySimpleProperty("parentNode", &impl_->parent_);
    RegisterProperty("childNodes", NewSlot(impl_, &DOMNodeImpl::GetChildNodes),
                     NULL);
    RegisterProperty("firstChild", NewSlot(impl_, &DOMNodeImpl::GetFirstChild),
                     NULL);
    RegisterProperty("lastChild", NewSlot(impl_, &DOMNodeImpl::GetLastChild),
                     NULL);
    RegisterProperty("previousSibling",
                     NewSlot(impl_, &DOMNodeImpl::GetPreviousSibling),
                     NULL);
    RegisterProperty("nextSibling",
                     NewSlot(impl_, &DOMNodeImpl::GetNextSibling),
                     NULL);
    RegisterProperty("attributes", NewSlot(this, &DOMNodeBase::GetAttributes),
                     NULL);
    RegisterConstant("ownerDocument", owner_document);
    RegisterProperty("text", NewSlot(this, &DOMNodeBase::GetTextContent),
                     NewSlot(this, &DOMNodeBase::SetTextContent));
    RegisterMethod("insertBefore",
                   NewSlot(impl_, &DOMNodeImpl::ScriptInsertBefore));
    RegisterMethod("replaceChild",
                   NewSlot(impl_, &DOMNodeImpl::ScriptReplaceChild));
    RegisterMethod("removeChild",
                   NewSlot(impl_, &DOMNodeImpl::ScriptRemoveChild));
    RegisterMethod("appendChild",
                   NewSlot(impl_, &DOMNodeImpl::ScriptAppendChild));
    RegisterMethod("hasChildNodes",
                   NewSlot(this, &DOMNodeBase::HasChildNodes));
    // Register slot of this class to allow subclasses to override.
    RegisterMethod("cloneNode", NewSlot(this, &DOMNodeBase::CloneNode));
    RegisterMethod("normalize", NewSlot(this, &DOMNodeBase::Normalize));

    SetPrototype(GlobalNode::Get());
  }

  virtual ~DOMNodeBase() {
    delete impl_;
    impl_ = NULL;
  }

  // Returns the address of impl_, to make it possible to access implementation
  // specific things through DOMNodeInterface pointers.
  virtual DOMNodeImpl *GetImpl() const { return impl_; }

  // Implements ScriptableInterface::Attach().
  virtual ScriptableInterface::OwnershipPolicy Attach() {
    impl_->AttachMulti(1);
    return ScriptableInterface::OWNERSHIP_SHARED;
  }
  // Implements ScriptableInterface::Detach().
  virtual bool Detach() {
    return impl_->DetachMulti(1, false);
  }
  virtual void TransientDetach() { impl_->DetachMulti(1, true); }

  virtual const char *GetNodeName() const { return impl_->name_.c_str(); }
  virtual const char *GetNodeValue() const { return NULL; }
  virtual void SetNodeValue(const char *node_value) { }
  // GetNodeType() is still abstract.

  virtual DOMNodeInterface *GetParentNode() { return impl_->parent_; }
  virtual const DOMNodeInterface *GetParentNode() const {
    return impl_->parent_;
  }

  virtual DOMNodeListInterface *GetChildNodes() {
    return impl_->GetChildNodes();
  }
  virtual const DOMNodeListInterface *GetChildNodes() const {
    return impl_->GetChildNodes();
  }

  virtual DOMNodeInterface *GetFirstChild() {
    return impl_->GetFirstChild();
  }
  virtual const DOMNodeInterface *GetFirstChild() const {
    return impl_->GetFirstChild();
  }
  virtual DOMNodeInterface *GetLastChild() {
    return impl_->GetLastChild();
  }
  virtual const DOMNodeInterface *GetLastChild() const {
    return impl_->GetLastChild();
  }

  virtual DOMNodeInterface *GetPreviousSibling() {
    return impl_->GetPreviousSibling();
  }
  virtual const DOMNodeInterface *GetPreviousSibling() const {
    return impl_->GetPreviousSibling();
  }
  virtual DOMNodeInterface *GetNextSibling() {
    return impl_->GetNextSibling();
  }
  virtual const DOMNodeInterface *GetNextSibling() const {
    return impl_->GetNextSibling();
  }

  virtual DOMNamedNodeMapInterface *GetAttributes() { return NULL; }
  virtual const DOMNamedNodeMapInterface *GetAttributes() const { return NULL; }

  virtual DOMDocumentInterface *GetOwnerDocument() {
    return impl_->owner_document_;
  }
  virtual const DOMDocumentInterface *GetOwnerDocument() const {
    return impl_->owner_document_;
  }

  virtual DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                        DOMNodeInterface *ref_child) {
    return impl_->InsertBefore(new_child, ref_child);
  }

  virtual DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                        DOMNodeInterface *old_child) {
    return impl_->ReplaceChild(new_child, old_child);
  }

  virtual DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) {
    return impl_->RemoveChild(old_child);
  }

  virtual DOMExceptionCode AppendChild(DOMNodeInterface *new_child) {
    return impl_->InsertBefore(new_child, NULL);
  }

  virtual bool HasChildNodes() const {
    return !impl_->children_.empty();
  }

  virtual DOMNodeInterface *CloneNode(bool deep) const {
    return impl_->CloneNode(deep);
  }

  virtual void Normalize() {
    impl_->Normalize();
  }

  virtual DOMNodeListInterface *GetElementsByTagName(const char *name) {
    return new ElementsByTagName(this, name);
  }

  virtual const DOMNodeListInterface *GetElementsByTagName(
      const char *name) const {
    return new ElementsByTagName(const_cast<DOMNodeBase *>(this), name);
  }

  virtual const char *GetTextContent() const {
    const char *content = GetNodeValue();
    return content ? content : impl_->GetChildrenTextContent();
  }

  virtual void SetTextContent(const char *text_content) {
    if (GetNodeValue())
      SetNodeValue(text_content);
    else
      impl_->SetChildTextContent(text_content);
  }

  virtual const char *GetXML() const {
    return impl_->GetXML();
  }

 private:
  DOMNodeImpl *impl_;
};

static const UTF16Char kBlankUTF16Str[] = { 0 };

template <typename Interface1>
class DOMCharacterData : public DOMNodeBase<Interface1> {
 public:
  DOMCharacterData(DOMDocumentInterface *owner_document,
                   const char *name, const UTF16Char *data)
      : DOMNodeBase<Interface1>(owner_document, name),
        data_(data ? data : kBlankUTF16Str) {
    RegisterProperty("data", NewSlot(this, &DOMCharacterData::GetData),
                     NewSlot(this, &DOMCharacterData::SetData));
    RegisterProperty("length", NewSlot(this, &DOMCharacterData::GetLength),
                     NULL);
    RegisterMethod("substringData",
                   NewSlot(this, &DOMCharacterData::ScriptSubstringData));
    RegisterMethod("appendData", NewSlot(this, &DOMCharacterData::AppendData));
    RegisterMethod("insertData",
                   NewSlot(this, &DOMCharacterData::ScriptInsertData));
    RegisterMethod("deleteData",
                   NewSlot(this, &DOMCharacterData::ScriptDeleteData));
    RegisterMethod("replaceData",
                   NewSlot(this, &DOMCharacterData::ScriptReplaceData));
  }

  virtual const char *GetNodeValue() const {
    if (utf8_data_.empty() && !data_.empty())
      ConvertStringUTF16ToUTF8(data_, &utf8_data_);
    return utf8_data_.c_str();
  }
  virtual void SetNodeValue(const char *value) {
    if (!value) value = "";
    data_.clear();
    ConvertStringUTF8ToUTF16(value, strlen(value), &data_);
    utf8_data_.clear();
  }

  virtual const UTF16Char *GetData() const { return data_.c_str(); }
  virtual void SetData(const UTF16Char *data) {
    data_ = data ? data : kBlankUTF16Str;
    utf8_data_.clear();
  }
  virtual size_t GetLength() const { return data_.length(); }

  virtual DOMExceptionCode SubstringData(size_t offset, size_t count,
                                         UTF16Char **result) const {
    ASSERT(result);
    *result = NULL;
    if (offset > data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(data_.length() - offset, count);

    *result = new UTF16Char[count + 1];
    memcpy(*result, data_.c_str() + offset, count * sizeof(UTF16Char));
    (*result)[count] = 0;
    return DOM_NO_ERR;
  }

  virtual void AppendData(const UTF16Char *arg) {
    if (arg) {
      data_ += arg;
      utf8_data_.clear();
    }
  }

  virtual DOMExceptionCode InsertData(size_t offset, const UTF16Char *arg) {
    if (offset > data_.length())
      return DOM_INDEX_SIZE_ERR;

    if (arg) {
      data_.insert(offset, arg);
      utf8_data_.clear();
    }
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode DeleteData(size_t offset, size_t count) {
    if (offset > data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(data_.length() - offset, count);

    data_.erase(offset, count);
    utf8_data_.clear();
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode ReplaceData(size_t offset, size_t count,
                                       const UTF16Char *arg) {
    if (offset > data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(data_.length() - offset, count);

    data_.replace(offset, count, arg ? arg : kBlankUTF16Str);
    utf8_data_.clear();
    return DOM_NO_ERR;
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    // DOMCharacterData doesn't allow adding children to it.
    return DOM_HIERARCHY_REQUEST_ERR;
  }

 private:
  UTF16String ScriptSubstringData(size_t offset, size_t count) {
    UTF16Char *result = NULL;
    CheckException(SubstringData(offset, count, &result));
    UTF16String str_result(result ? result : kBlankUTF16Str);
    delete [] result;
    return str_result;
  }

  void ScriptInsertData(size_t offset, const UTF16Char *arg) {
    CheckException(InsertData(offset, arg));
  }

  void ScriptDeleteData(size_t offset, size_t count) {
    CheckException(DeleteData(offset, count));
  }

  void ScriptReplaceData(size_t offset, size_t count, const UTF16Char *arg) {
    CheckException(ReplaceData(offset, count, arg));
  }

  UTF16String data_;
  mutable std::string utf8_data_; // Cache of data_ in UTF8 format.
};

class DOMElement;
class DOMAttr : public DOMNodeBase<DOMAttrInterface> {
 public:
  DEFINE_CLASS_ID(0x5fee553d317b47d9, DOMAttrInterface);
  typedef DOMNodeBase<DOMAttrInterface> Super;

  DOMAttr(DOMDocumentInterface *owner_document, const char *name,
          DOMElement *owner_element)
      : Super(owner_document, name),
        owner_element_(NULL) {
    SetOwnerElement(owner_element);

    RegisterProperty("name", NewSlot(this, &DOMAttr::GetName), NULL);
    // Our DOMAttrs are always specified, because we don't support DTD for now.
    RegisterConstant("specified", true);
    RegisterProperty("value", NewSlot(this, &DOMAttr::GetValue),
                     NewSlot(this, &DOMAttr::SetValue));
    // ownerElement is a DOM2 property, so not registered for now.
  }

  virtual DOMNodeInterface *CloneNode(bool deep) const {
    // Attr.cloneNode always clone its children, even if deep is false.
    return Super::CloneNode(true);
  }

  virtual const char *GetNodeValue() const {
    return GetImpl()->GetChildrenTextContent();
  }
  virtual void SetNodeValue(const char *value) {
    GetImpl()->SetChildTextContent(value);
  }
  virtual NodeType GetNodeType() const { return ATTRIBUTE_NODE; }

  virtual const char *GetName() const { return GetNodeName(); }
  // Our DOMAttrs are always specified, because we don't support DTD for now.
  virtual bool IsSpecified() const { return true; }
  virtual const char *GetValue() const { return GetNodeValue(); }
  virtual void SetValue(const char *value) { SetNodeValue(value); }

  virtual DOMElementInterface *GetOwnerElement(); // Defined later.
  virtual const DOMElementInterface *GetOwnerElement() const;

  void SetOwnerElement(DOMElement *owner_element); // Defined after DOMElement.

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Omit the indent parameter, let the parent deal with it.
    xml->append(GetNodeName());
    xml->append("=\"");
    xml->append(EncodeXMLString(GetNodeValue()));
    xml->append(1, '"');
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR) {
      NodeType type = new_child->GetNodeType();
      if (type != TEXT_NODE && type != ENTITY_REFERENCE_NODE)
        code = DOM_HIERARCHY_REQUEST_ERR;
    }
    return code;
  }

  virtual DOMNodeInterface *CloneSelf() {
    // The content will be cloned by common CloneNode implementation,
    // because for Attr.cloneNode(), children are always cloned.
    return new DOMAttr(GetOwnerDocument(), GetName(), NULL);
  }

 private:
  DOMElement *owner_element_;
};

class DOMElement : public DOMNodeBase<DOMElementInterface> {
 public:
  DEFINE_CLASS_ID(0x721f40f59a3f48a9, DOMElementInterface);
  typedef DOMNodeBase<DOMElementInterface> Super;
  typedef std::vector<DOMAttr *> Attrs;

  DOMElement(DOMDocumentInterface *owner_document, const char *tag_name)
      : Super(owner_document, tag_name) {
    RegisterProperty("tagName", NewSlot(this, &DOMElement::GetTagName), NULL);
    RegisterMethod("getAttribute", NewSlot(this, &DOMElement::GetAttribute));
    RegisterMethod("setAttribute",
                   NewSlot(this, &DOMElement::ScriptSetAttribute));
    RegisterMethod("removeAttribute",
                   NewSlot(this, &DOMElement::RemoveAttribute));
    RegisterMethod("getAttributeNode",
                   NewSlot(this, &DOMElement::GetAttributeNode));
    RegisterMethod("setAttributeNode",
                   NewSlot(this, &DOMElement::ScriptSetAttributeNode));
    RegisterMethod("removeAttributeNode",
                   NewSlot(this, &DOMElement::ScriptRemoveAttributeNode));
    RegisterMethod("getElementsByTagName",
                   NewSlot(implicit_cast<Super *>(this),
                           &Super::GetElementsByTagName));
  }

  ~DOMElement() {
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      delete (*it);
    attrs_.clear();
  }

  virtual NodeType GetNodeType() const { return ELEMENT_NODE; }
  virtual const char *GetTagName() const { return GetNodeName(); }

  virtual void Normalize() {
    Super::Normalize();
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      (*it)->Normalize();
  }

  virtual const char *GetAttribute(const char *name) const {
    Attrs::const_iterator it = FindAttr(name);
    // TODO: Default value logic.
    return it == attrs_.end() ? "" : (*it)->GetValue();
  }

  virtual DOMExceptionCode SetAttribute(const char *name, const char *value) {
    if (!CheckXMLName(name))
      return DOM_INVALID_CHARACTER_ERR;

    Attrs::iterator it = FindAttr(name);
    if (it == attrs_.end()) {
      DOMAttr *attr = new DOMAttr(GetOwnerDocument(), name, this);
      attrs_.push_back(attr);
      attr->SetValue(value);
    } else {
      (*it)->SetValue(value);
    }
    return DOM_NO_ERR;
  }

  virtual void RemoveAttribute(const char *name) {
    if (name)
      RemoveAttributeInternal(name);
  }

  virtual DOMAttrInterface *GetAttributeNode(const char *name) {
    return const_cast<DOMAttrInterface *>(
        static_cast<const DOMElement *>(this)->GetAttributeNode(name));
  }

  virtual const DOMAttrInterface *GetAttributeNode(const char *name) const {
    Attrs::const_iterator it = FindAttr(name);
    return it == attrs_.end() ? NULL : *it;
  }

  virtual DOMExceptionCode SetAttributeNode(DOMAttrInterface *new_attr) {
    if (!new_attr)
      return DOM_NULL_POINTER_ERR;
    if (new_attr->GetOwnerDocument() != GetOwnerDocument())
      return DOM_WRONG_DOCUMENT_ERR;
    if (new_attr->GetOwnerElement()) {
      return new_attr->GetOwnerElement() != this ?
             DOM_INUSE_ATTRIBUTE_ERR : DOM_NO_ERR;
    }

    Attrs::iterator it = FindAttr(new_attr->GetName());
    if (it != attrs_.end()) {
      (*it)->SetOwnerElement(NULL);
      attrs_.erase(it);
    }

    DOMAttr *new_attr_internal = down_cast<DOMAttr *>(new_attr);
    new_attr_internal->SetOwnerElement(this);
    attrs_.push_back(new_attr_internal);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode RemoveAttributeNode(DOMAttrInterface *old_attr) {
    if (!old_attr)
      return DOM_NULL_POINTER_ERR;
    if (old_attr->GetOwnerElement() != this)
      return DOM_NOT_FOUND_ERR;

    Attrs::iterator it = FindAttrNode(old_attr);
    (*it)->SetOwnerElement(NULL);
    attrs_.erase(it);
    return DOM_NO_ERR;
  }

  virtual DOMNamedNodeMapInterface *GetAttributes() {
    return new AttrsNamedMap(this);
  }
  virtual const DOMNamedNodeMapInterface *GetAttributes() const {
    return new AttrsNamedMap(const_cast<DOMElement *>(this));
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    std::string::size_type line_begin = xml->length();
    AppendIndentNewLine(indent, xml);
    xml->append(1, '<');
    xml->append(GetNodeName());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
      xml->append(1, ' ');
      (*it)->AppendXML(indent, xml);
      if (xml->size() - line_begin > kLineLengthThreshold) {
        line_begin = xml->length();
        AppendIndentNewLine(indent + kIndent, xml);
      }
    }
    if (HasChildNodes()) {
      xml->append(1, '>');
      GetImpl()->AppendChildrenXML(indent + kIndent, xml);
      AppendIndentIfNewLine(indent, xml);
      xml->append("</");
      xml->append(GetNodeName());
      xml->append(">\n");
    } else {
      xml->append("/>\n");
    }
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR)
      code = CheckCommonChildType(new_child);
    return code;
  }

  virtual DOMNodeInterface *CloneSelf() {
    DOMElement *element = new DOMElement(GetOwnerDocument(), GetTagName());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
      DOMAttrInterface *cloned_attr =
          down_cast<DOMAttrInterface *>((*it)->CloneNode(true));
      element->SetAttributeNode(cloned_attr);
    }
    return element;
  }

 private:
  class AttrsNamedMap : public ScriptableHelper<DOMNamedNodeMapInterface> {
   public:
    DEFINE_CLASS_ID(0xbe2998ee79754343, DOMNamedNodeMapInterface)
    AttrsNamedMap(DOMElement *element) : element_(element) {
      DOMNamedNodeMapInterface *super_ptr =
          implicit_cast<DOMNamedNodeMapInterface *>(this);
      RegisterProperty("length",
                       NewSlot(super_ptr, &DOMNamedNodeMapInterface::GetLength),
                       NULL);
      RegisterMethod("getNamedItem",
                     NewSlot(super_ptr,
                             &DOMNamedNodeMapInterface::GetNamedItem));
      RegisterMethod("setNamedItem",
                     NewSlot(this, &AttrsNamedMap::ScriptSetNamedItem));
      RegisterMethod("removeNamedItem",
                     NewSlot(this, &AttrsNamedMap::ScriptRemoveNamedItem));
      RegisterMethod("item",
                     NewSlot(super_ptr, &DOMNamedNodeMapInterface::GetItem));
    }

    // AttrsNamedMap is not reference counted. The Attach() and Detach()
    // methods are only for the script adapter.
    virtual OwnershipPolicy Attach() {
      element_->Attach();
      return OWNERSHIP_TRANSFERRABLE; 
    }
    virtual bool Detach() {
      element_->Detach();
      delete this;
      return true;
    }

    virtual DOMNodeInterface *GetNamedItem(const char *name) {
      return element_->GetAttributeNode(name);
    }
    virtual const DOMNodeInterface *GetNamedItem(const char *name) const {
      return element_->GetAttributeNode(name);
    }
    virtual DOMExceptionCode SetNamedItem(DOMNodeInterface *arg) {
      if (!arg)
        return DOM_NULL_POINTER_ERR;
      if (arg->GetNodeType() != ATTRIBUTE_NODE)
        return DOM_HIERARCHY_REQUEST_ERR;
      return element_->SetAttributeNode(down_cast<DOMAttrInterface *>(arg));
    }
    virtual DOMExceptionCode RemoveNamedItem(const char *name) {
      if (!name)
        return DOM_NULL_POINTER_ERR;
      return element_->RemoveAttributeInternal(name) ?
             DOM_NO_ERR : DOM_NOT_FOUND_ERR;
    }
    virtual DOMNodeInterface *GetItem(size_t index) {
      return index < element_->attrs_.size() ? element_->attrs_[index] : NULL;
    }
    virtual const DOMNodeInterface *GetItem(size_t index) const {
      return index < element_->attrs_.size() ? element_->attrs_[index] : NULL;
    }
    virtual size_t GetLength() const {
      return element_->attrs_.size();
    }

   private:
    DOMNodeInterface *ScriptSetNamedItem(DOMNodeInterface *arg) {
      if (!arg)
        CheckException(DOM_NULL_POINTER_ERR);
      if (arg->GetNodeType() != ATTRIBUTE_NODE)
        CheckException(DOM_HIERARCHY_REQUEST_ERR);
      return element_->ScriptSetAttributeNode(
          down_cast<DOMAttrInterface *>(arg));
    }

    DOMNodeInterface *ScriptRemoveNamedItem(const char *name) {
      DOMNodeInterface *removed_node = GetNamedItem(name);
      if (removed_node)
        removed_node->Attach();
      DOMExceptionCode code = RemoveNamedItem(name);
      if (removed_node)
        removed_node->GetImpl()->DetachMulti(1, code == DOM_NO_ERR);
      CheckException(code);
      return removed_node;
    }

    DOMElement *element_;
  };

  void ScriptSetAttribute(const char *name, const char *value) {
    CheckException(SetAttribute(name, value));
  }

  DOMAttrInterface *ScriptSetAttributeNode(DOMAttrInterface *new_attr) {
    DOMAttrInterface *replaced_attr = NULL;
    if (new_attr) {
      // Add a temporary reference to the replaced attr to prevent it from
      // being deleted in SetAttributeNode().
      replaced_attr = GetAttributeNode(new_attr->GetName());
      if (replaced_attr)
        replaced_attr->Attach();
    }
    DOMExceptionCode code = SetAttributeNode(new_attr);
    if (replaced_attr) {
      // Remove the temporary reference transiently if no error.
      replaced_attr->GetImpl()->DetachMulti(1, code == DOM_NO_ERR);
    }
    CheckException(code);
    return replaced_attr;
  }

  DOMAttrInterface *ScriptRemoveAttributeNode(DOMAttrInterface *old_attr) {
    CheckException(RemoveAttributeNode(old_attr));
    return old_attr;
  }

  bool RemoveAttributeInternal(const char *name) {
    Attrs::iterator it = FindAttr(name);
    if (it != attrs_.end()) {
      (*it)->SetOwnerElement(NULL);
      attrs_.erase(it);
      return true;
    }
    return false;
    // TODO: Deal with default values if we support DTD.
  }

  Attrs::iterator FindAttr(const char *name) {
    ASSERT(name);
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      if (strcmp((*it)->GetName(), name) == 0)
        return it;
    return attrs_.end();
  }

  Attrs::iterator FindAttrNode(DOMAttrInterface *attr) {
    ASSERT(attr && attr->GetOwnerElement() == this);
    Attrs::iterator it = std::find(attrs_.begin(), attrs_.end(), attr);
    ASSERT(it != attrs_.end());
    return it;
  }

  Attrs::const_iterator FindAttr(const char *name) const {
    ASSERT(name);
    for (Attrs::const_iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      if (strcmp((*it)->GetName(), name) == 0)
        return it;
    return attrs_.end();
  }

  Attrs::const_iterator FindAttrNode(DOMAttrInterface *attr) const {
    ASSERT(attr && attr->GetOwnerElement() == this);
    Attrs::const_iterator it = std::find(attrs_.begin(), attrs_.end(), attr);
    ASSERT(it != attrs_.end());
    return it;
  }

  std::string tag_name_;
  Attrs attrs_;
};

DOMElementInterface *DOMAttr::GetOwnerElement() {
  return owner_element_;
}
const DOMElementInterface *DOMAttr::GetOwnerElement() const {
  return owner_element_;
}

void DOMAttr::SetOwnerElement(DOMElement *owner_element) {
  if (owner_element_ != owner_element) {
    owner_element_ = owner_element;
    GetImpl()->SetOwnerNode(owner_element);
  }
}

static DOMExceptionCode DoSplitText(DOMTextInterface *text,
                                    size_t offset,
                                    DOMTextInterface **new_text) {
  ASSERT(new_text);
  *new_text = NULL;
  if (offset > text->GetLength())
    return DOM_INDEX_SIZE_ERR;

  size_t tail_size = text->GetLength() - offset;
  UTF16Char *tail_data;
  text->SubstringData(offset, tail_size, &tail_data);
  *new_text = down_cast<DOMTextInterface *>(text->CloneNode(false));
  (*new_text)->SetData(tail_data);
  text->DeleteData(offset, tail_size);
  delete [] tail_data;

  if (text->GetParentNode())
    text->GetParentNode()->InsertBefore(*new_text, text->GetNextSibling());
  return DOM_NO_ERR;
}

class DOMText : public DOMCharacterData<DOMTextInterface> {
 public:
  DEFINE_CLASS_ID(0xdcd93e1ac43b49d2, DOMTextInterface);
  typedef DOMCharacterData<DOMTextInterface> Super;

  DOMText(DOMDocumentInterface *owner_document, const UTF16Char *data)
      : Super(owner_document, kDOMTextName, data) {
    RegisterMethod("splitText", NewSlot(this, &DOMText::ScriptSplitText));
  }

  virtual NodeType GetNodeType() const { return TEXT_NODE; }

  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) {
    return DoSplitText(this, offset, new_text);
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Omit the indent parameter, let the parent deal with it.
    std::string node_value(GetNodeValue());
    std::string trimmed = TrimString(EncodeXMLString(GetNodeValue()));
    if (!node_value.empty() &&
        (trimmed.empty() ||
         *(node_value.end() - 1) != *(trimmed.end() - 1))) {
      // The tail of the text has been trimmed.
      NodeType next_type = GetNextSibling() ?
                           GetNextSibling()->GetNodeType() : ELEMENT_NODE;
      if (next_type == TEXT_NODE || next_type == ENTITY_REFERENCE_NODE)
        // Preserve one space.
        trimmed += ' ';
    }
    xml->append(trimmed);
  }

 protected:
  virtual DOMNodeInterface *CloneSelf() {
    return new DOMText(GetOwnerDocument(), GetData());
  }

 private:
  DOMTextInterface *ScriptSplitText(size_t offset) {
    DOMTextInterface *result = NULL;
    CheckException(SplitText(offset, &result));
    return result;
  }
};

class DOMComment : public DOMCharacterData<DOMCommentInterface> {
 public:
  DEFINE_CLASS_ID(0x8f177233373d4015, DOMCommentInterface);
  typedef DOMCharacterData<DOMCommentInterface> Super;

  DOMComment(DOMDocumentInterface *owner_document, const UTF16Char *data)
      : Super(owner_document, kDOMCommentName, data) {
  }

  virtual NodeType GetNodeType() const { return COMMENT_NODE; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Omit the indent parameter, let the parent deal with it.
    AppendIndentNewLine(indent, xml);
    xml->append("<!--");
    xml->append(EncodeXMLString(GetNodeValue()));
    xml->append("-->\n");
  }

 protected:
  virtual DOMNodeInterface *CloneSelf() {
    return new DOMComment(GetOwnerDocument(), GetData());
  }
};

class DOMCDATASection : public DOMCharacterData<DOMCDATASectionInterface> {
 public:
  DEFINE_CLASS_ID(0xe6b4c9779b3d4127, DOMCDATASectionInterface);
  typedef DOMCharacterData<DOMCDATASectionInterface> Super;

  DOMCDATASection(DOMDocumentInterface *owner_document, const UTF16Char *data)
      : Super(owner_document, kDOMCDATASectionName, data) {
    // TODO:
  }

  virtual NodeType GetNodeType() const { return CDATA_SECTION_NODE; }

  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) {
    return DoSplitText(this, offset, new_text);
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Omit the indent parameter, let the parent deal with it.
    AppendIndentNewLine(indent, xml);
    xml->append("<![CDATA[");
    xml->append(GetNodeValue());
    xml->append("]]>\n");
  }

 protected:
  virtual DOMNodeInterface *CloneSelf() {
    return new DOMCDATASection(GetOwnerDocument(), GetData());
  }
};

class DOMDocumentFragment : public DOMNodeBase<DOMDocumentFragmentInterface> {
 public:
  DEFINE_CLASS_ID(0xe6b4c9779b3d4127, DOMDocumentFragmentInterface);
  typedef DOMNodeBase<DOMDocumentFragmentInterface> Super;

  DOMDocumentFragment(DOMDocumentInterface *owner_document)
      : Super(owner_document, kDOMDocumentFragmentName) { }

  virtual NodeType GetNodeType() const { return DOCUMENT_FRAGMENT_NODE; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Because DOMDocumentFragment can't be child of any node, the indent
    // should always be zero.
    ASSERT(indent == 0);
    GetImpl()->AppendChildrenXML(0, xml);
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR)
      code = CheckCommonChildType(new_child);
    return code;
  }

  virtual DOMNodeInterface *CloneSelf() {
    return new DOMDocumentFragment(GetOwnerDocument());
  }
};

class DOMProcessingInstruction
    : public DOMNodeBase<DOMProcessingInstructionInterface> {
 public:
  DEFINE_CLASS_ID(0x54e1e0de36a2464f, DOMProcessingInstructionInterface);
  typedef DOMNodeBase<DOMProcessingInstructionInterface> Super;

  DOMProcessingInstruction(DOMDocumentInterface *owner_document,
                           const char *target, const char *data)
      : Super(owner_document, target),
        target_(target ? target : ""),
        data_(data ? data : "") {
    RegisterConstant("target", target_);
    RegisterProperty("data", NewSlot(this, &DOMProcessingInstruction::GetData),
                     NewSlot(this, &DOMProcessingInstruction::SetData));
  }

  virtual const char *GetNodeValue() const { return GetData(); }
  virtual void SetNodeValue(const char *value) { SetData(value); }
  virtual NodeType GetNodeType() const { return PROCESSING_INSTRUCTION_NODE; }

  virtual const char *GetTarget() const { return target_.c_str(); }
  virtual const char *GetData() const { return data_.c_str(); }
  virtual void SetData(const char *data) { data_ = data ? data : ""; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    AppendIndentNewLine(indent, xml);
    xml->append("<?");
    xml->append(GetNodeName());
    xml->append(1, ' ');
    xml->append(GetNodeValue());
    xml->append("?>\n");
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    // DOMProcessingInstruction doesn't allow adding children to it.
    return DOM_HIERARCHY_REQUEST_ERR;
  }

  virtual DOMNodeInterface *CloneSelf() {
    return new DOMDocumentFragment(GetOwnerDocument());
  }

 private:
  std::string target_;
  std::string data_;
};

class DOMImplementation : public ScriptableHelper<DOMImplementationInterface> {
 public:
  DEFINE_CLASS_ID(0xd23149a89cf24e12, DOMImplementationInterface);

  DOMImplementation() {
    RegisterMethod("hasFeature", NewSlot(this, &DOMImplementation::HasFeature));
  }

  virtual bool HasFeature(const char *feature, const char *version) const {
    return feature && strcasecmp(feature, "XML") == 0 &&
          (!version || !version[0] || strcmp(version, "1.0") == 0);
  }
};

// Note about reference counting of DOMDocument: the reference count is the sum
// of the following two counts:
// 1. Normal reference count same as DOMNodeBase, that is, the accumulated
//    reference counts of all descendants;
// 2. Count of all orphan trees. This count will be increased when a new
//    orphan node is created, and be decreased when a root of an orphan tree
//    is added into another tree, or an orphan is deleted.
class DOMDocument : public DOMNodeBase<DOMDocumentInterface> {
 public:
  DEFINE_CLASS_ID(0x23dffa4b4f234226, DOMDocumentInterface);
  typedef DOMNodeBase<DOMDocumentInterface> Super;

  DOMDocument() : Super(NULL, kDOMDocumentName) {
    RegisterConstant("doctype", static_cast<ScriptableInterface *>(NULL));
    RegisterConstant("implementation", &implementation_);
    RegisterProperty("documentElement",
                     NewSlot(this, &DOMDocument::GetDocumentElement), NULL);
    RegisterMethod("loadXML", NewSlot(this, &DOMDocument::LoadXML));
    RegisterMethod("createElement",
                   NewSlot(this, &DOMDocument::ScriptCreateElement));
    RegisterMethod("createDocumentFragment",
                   NewSlot(this, &DOMDocument::CreateDocumentFragment));
    RegisterMethod("createTextNode",
                   NewSlot(this, &DOMDocument::CreateTextNode));
    RegisterMethod("createComment", NewSlot(this, &DOMDocument::CreateComment));
    RegisterMethod("createCDATASection",
                   NewSlot(this, &DOMDocument::CreateCDATASection));
    RegisterMethod("createTextNode",
                   NewSlot(this, &DOMDocument::CreateTextNode));
    RegisterMethod("createProcessingInstruction",
                   NewSlot(this,
                           &DOMDocument::ScriptCreateProcessingInstruction));
    RegisterMethod("createAttribute",
                   NewSlot(this, &DOMDocument::ScriptCreateAttribute));
    RegisterMethod("createEntityReference",
                   NewSlot(this, &DOMDocument::ScriptCreateEntityReference));
    RegisterMethod("getElementsByTagName",
                   NewSlot(implicit_cast<Super *>(this),
                           &Super::GetElementsByTagName));
  }

  virtual bool LoadXML(const char *xml) {
    GetImpl()->RemoveAllChildren();
    return ParseXMLIntoDOM(xml, "NONAME", this, NULL);
  }

  virtual NodeType GetNodeType() const { return DOCUMENT_NODE; }
  virtual DOMDocumentTypeInterface *GetDoctype() {
    return NULL;
    // TODO: If we support DTD.
    // return down_cast<DOMDocumentTypeInterface *>(
    //     FindNodeOfType(DOCUMENT_TYPE_NODE));
  }
  virtual const DOMDocumentTypeInterface *GetDoctype() const {
    return NULL;
    // TODO: If we support DTD.
    // return down_cast<DOMDocumentTypeInterface *>(
    //     FindNodeOfType(DOCUMENT_TYPE_NODE));
  }

  virtual DOMImplementationInterface *GetImplementation() {
    return &implementation_;
  }
  virtual const DOMImplementationInterface *GetImplementation() const {
    return &implementation_;
  }

  virtual DOMElementInterface *GetDocumentElement() {
    return const_cast<DOMElementInterface *>(
        down_cast<const DOMElementInterface *>(FindNodeOfType(ELEMENT_NODE)));
  }

  virtual const DOMElementInterface *GetDocumentElement() const {
    return down_cast<const DOMElementInterface *>(FindNodeOfType(ELEMENT_NODE));
  }

  virtual DOMExceptionCode CreateElement(const char *tag_name,
                                         DOMElementInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!CheckXMLName(tag_name))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMElement(this, tag_name);
    return DOM_NO_ERR;
  }

  virtual DOMDocumentFragmentInterface *CreateDocumentFragment() {
    return new DOMDocumentFragment(this);
  }

  virtual DOMTextInterface *CreateTextNode(const UTF16Char *data) {
    return new DOMText(this, data);
  }

  virtual DOMCommentInterface *CreateComment(const UTF16Char *data) {
    return new DOMComment(this, data);
  }

  virtual DOMCDATASectionInterface *CreateCDATASection(const UTF16Char *data) {
    return new DOMCDATASection(this, data);
  }

  virtual DOMExceptionCode CreateProcessingInstruction(
      const char *target, const char *data,
      DOMProcessingInstructionInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!CheckXMLName(target))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMProcessingInstruction(this, target, data);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode CreateAttribute(const char *name,
                                           DOMAttrInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!CheckXMLName(name))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMAttr(this, name, NULL);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode CreateEntityReference(
      const char *name, DOMEntityReferenceInterface **result) {
    ASSERT(result);
    *result = NULL;
    return DOM_NOT_SUPPORTED_ERR;
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    ASSERT(indent == 0);
    xml->append(kStandardXMLDecl);
    GetImpl()->AppendChildrenXML(0, xml);
  }

 protected:
  virtual DOMNodeInterface *CloneSelf() {
    return NULL;
  }

  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR) {
      NodeType type = new_child->GetNodeType();
      if (type == ELEMENT_NODE) {
        // Only one element node is allowed.
        if (GetDocumentElement()) {
          DLOG("DOMDocument::CheckNewChild: Duplicated document element");
          code = DOM_HIERARCHY_REQUEST_ERR;
        }
      } else if (type == DOCUMENT_TYPE_NODE) {
        // Only one doc type node is allowed.
        if (GetDoctype()) {
          DLOG("DOMDocument::CheckNewChild: Duplicated doctype");
          code = DOM_HIERARCHY_REQUEST_ERR;
        }
      } else if (type != PROCESSING_INSTRUCTION_NODE && type != COMMENT_NODE) {
        DLOG("DOMDocument::CheckNewChild: Invalid type of document child: %d",
             type);
        code = DOM_HIERARCHY_REQUEST_ERR;
      }
    }
    return code;
  }

 private:
  const DOMNodeInterface *FindNodeOfType(NodeType type) const {
    const DOMNodeListInterface *children = GetChildNodes();
    const DOMNodeInterface *result = NULL;
    size_t length = children->GetLength();
    for (size_t i = 0; i < length; i++) {
      const DOMNodeInterface *item = children->GetItem(i);
      if (item->GetNodeType() == type) {
        result = item;
        break;
      }
    }
    delete children;
    return result;
  }

  DOMElementInterface *ScriptCreateElement(const char *tag_name) {
    DOMElementInterface *result = NULL;
    CheckException(CreateElement(tag_name, &result));
    return result;
  }

  DOMProcessingInstructionInterface *ScriptCreateProcessingInstruction(
      const char *target, const char *data) {
    DOMProcessingInstructionInterface *result = NULL;
    CheckException(CreateProcessingInstruction(target, data, &result));
    return result;
  }

  DOMAttrInterface *ScriptCreateAttribute(const char *name) {
    DOMAttrInterface *result = NULL;
    CheckException(CreateAttribute(name, &result));
    return result;
  }

  ScriptableInterface *ScriptCreateEntityReference(const char *name) {
    // TODO: if we support DTD.
    return NULL;
  }

  DOMImplementation implementation_;
};

} // namespace internal

DOMDocumentInterface *CreateDOMDocument() {
  return new internal::DOMDocument();
}

void RegisterDOMGlobalScriptable(
    ScriptableHelper<ScriptableInterface> *scriptable) {
  scriptable->RegisterConstant("DOMException",
                               internal::GlobalException::Get());
  scriptable->RegisterConstant("Node", internal::GlobalNode::Get());
}

} // namespace ggadget
