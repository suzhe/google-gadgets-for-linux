/*
  Copyright 2008 Google Inc.

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
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_dom_interface.h>

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

class GlobalException : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x81f363ca1c034f39, ScriptableInterface);
  static GlobalException *Get() {
    static GlobalException global_exception;
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

class GlobalNode : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x2a9d299fb51c4070, ScriptableInterface);
  static GlobalNode *Get() {
    static GlobalNode global_node;
    return &global_node;
  }
 private:
  GlobalNode() {
    RegisterConstants(arraysize(kNodeTypeNames), kNodeTypeNames, NULL);
  }
};

class DOMException : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x6486921444b44784, ScriptableInterface);

  DOMException(DOMExceptionCode code) : code_(code) {
    SetInheritsFrom(GlobalException::Get());
  }

  virtual void DoClassRegister() {
    RegisterProperty("code", NewSlot(&DOMException::GetCode), NULL);
    RegisterMethod("toString", NewSlot(&DOMException::ToString));
  }

  std::string ToString() const {
    const char *exception_name = code_ >= 0 &&
        static_cast<size_t>(code_) < arraysize(kExceptionNames) ?
            kExceptionNames[code_] : "unknown";
    return StringPrintf("DOMException: %d(%s)", code_, exception_name);
  }

  DOMExceptionCode GetCode() const { return code_; }

 private:
  DOMExceptionCode code_;
};

// Used in the methods for script to throw an script exception on errors.
template <typename T>
static bool GlobalCheckException(T *owner, DOMExceptionCode code) {
  if (code != DOM_NO_ERR) {
    DLOG("Throw DOMException: %d", code);
    owner->SetPendingException(new DOMException(code));
    return false;
  }
  return true;
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
    SetArrayHandler(NewSlot(implicit_cast<DOMNodeListInterface *>(this),
                            &DOMNodeListInterface::GetItem),
                    NULL);
  }
  virtual void DoClassRegister() {
    RegisterProperty("length", NewSlot(&DOMNodeListInterface::GetLength), NULL);
    RegisterMethod("item", NewSlot(&DOMNodeListBase::GetItemNotConst));
  }
 private:
  DOMNodeInterface *GetItemNotConst(size_t index) { return GetItem(index); }
};

// The DOMNodeList used as the return value of GetElementsByTagName().
class ElementsByTagName : public DOMNodeListBase {
 public:
  DEFINE_CLASS_ID(0x08b36d84ae044941, DOMNodeListInterface);

  ElementsByTagName(DOMNodeInterface *node, const char *name)
      : node_(node),
        name_(name ? name : ""),
        wildcard_(name && name[0] == '*' && name[1] == '\0') {
    node->Ref();
  }
  virtual ~ElementsByTagName() {
    node_->Unref();
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
    const DOMNodeInterface *result_item = NULL;
    for (const DOMNodeInterface *item = node->GetFirstChild(); item;
         item = item->GetNextSibling()) {
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
    return result_item;
  }

  size_t CountChildElements(const DOMNodeInterface *node) const {
    size_t count = 0;
    for (const DOMNodeInterface *item = node->GetFirstChild(); item;
         item = item->GetNextSibling()) {
      if (item->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
        if (wildcard_ || name_ == item->GetNodeName())
          count++;
        count += CountChildElements(item);
      }
    }
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
  virtual bool CheckException(DOMExceptionCode code) = 0;
};

class DOMNodeImpl {
 public:
  typedef std::vector<DOMNodeInterface *> Children;

  DOMNodeImpl(DOMNodeInterface *node,
              DOMNodeImplCallbacks *callbacks,
              DOMDocumentInterface *owner_document,
              const char *name)
      : node_(node),
        callbacks_(callbacks),
        owner_document_(owner_document),
        parent_(NULL),
        owner_node_(NULL),
        previous_sibling_(NULL), next_sibling_(NULL),
        row_(0), column_(0) {
    ASSERT(name && *name);
    if (!SplitString(name, ":", &prefix_, &local_name_)) {
      ASSERT(local_name_.empty());
      local_name_.swap(prefix_);
    }
    // Pointer comparison is intended here.
    if (name != kDOMDocumentName) {
      ASSERT(owner_document_);
      // Any newly created node has no parent and thus is orphan. Increase the
      // document orphan count.
      owner_document_->Ref();
    }
  }

  virtual ~DOMNodeImpl() {
    if (!owner_node_ && owner_document_) {
      // The node is still an orphan. Remove the document orphan count.
      owner_document_->Unref();
    }

    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      // At this time, the refcount of all children should have already been
      // reached 0 but not deleted because the last ref was removed transiently.
      delete *it;
    }
    children_.clear();
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
    return previous_sibling_ ? previous_sibling_->node_ : NULL;
  }
  DOMNodeInterface *GetNextSibling() {
    return next_sibling_ ? next_sibling_->node_ : NULL;
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
      // Add a temporary ref to prevent new_child from being deleted.
      new_child->Ref();
      old_parent->RemoveChild(new_child);
      new_child->Unref(true);
    }

    DOMNodeImpl *new_child_impl = new_child->GetImpl();
    DOMNodeImpl *prev_child_impl = NULL;
    if (ref_child) {
      DOMNodeImpl *ref_child_impl = ref_child->GetImpl();
      if (ref_child_impl->previous_sibling_)
        prev_child_impl = ref_child_impl->previous_sibling_;
      new_child_impl->next_sibling_ = ref_child_impl;
      ref_child_impl->previous_sibling_ = new_child_impl;
      children_.insert(FindChild(ref_child), new_child);
    } else {
      if (!children_.empty())
        prev_child_impl = children_.back()->GetImpl();
      children_.push_back(new_child);
    }
    if (prev_child_impl) {
      prev_child_impl->next_sibling_ = new_child_impl;
      new_child_impl->previous_sibling_ = prev_child_impl;
    }

    new_child_impl->SetParent(node_);
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
    DOMNodeImpl *old_child_impl = old_child->GetImpl();
    DOMNodeImpl *prev_child_impl = old_child_impl->previous_sibling_;
    DOMNodeImpl *next_child_impl = old_child_impl->next_sibling_;
    if (prev_child_impl)
      prev_child_impl->next_sibling_ = next_child_impl;
    if (next_child_impl)
      next_child_impl->previous_sibling_ = prev_child_impl;
    old_child_impl->previous_sibling_ = NULL;
    old_child_impl->next_sibling_ = NULL;
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
            text0->InsertData(text0->GetLength(), text->GetData().c_str());
            RemoveChild(text);
            i--;
          }
        }
      } else {
        child->Normalize();
      }
    }
  }

  std::string GetChildrenTextContent() {
    std::string result;
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      DOMNodeInterface::NodeType type = (*it)->GetNodeType();
      if (type != DOMNodeInterface::COMMENT_NODE &&
          type != DOMNodeInterface::PROCESSING_INSTRUCTION_NODE)
        result += (*it)->GetTextContent();
    }
    return result;
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

  std::string GetXML() {
    std::string result;
    callbacks_->AppendXML(0, &result);
    return result;
  }

  std::string GetNodeName() {
    return prefix_.empty() ? local_name_ : prefix_ + ":" + local_name_;
  }

  DOMExceptionCode SetPrefix(const char *prefix) {
    if (!prefix || !*prefix) {
      prefix_.clear();
    } else if (owner_document_->GetXMLParser()->CheckXMLName(prefix)) {
      prefix_ = prefix;
    } else {
      return DOM_INVALID_CHARACTER_ERR;
    }
    return DOM_NO_ERR;
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
         it != children_.end(); ++it) {
      DOMNodeImpl *child_impl = (*it)->GetImpl();
      child_impl->previous_sibling_ = NULL;
      child_impl->next_sibling_ = NULL;
      (*it)->GetImpl()->SetParent(NULL);
    }
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
    return callbacks_->CheckException(InsertBefore(new_child, ref_child))?
           new_child : NULL;
  }

  DOMNodeInterface *ScriptReplaceChild(DOMNodeInterface *new_child,
                                       DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Ref();
    DOMExceptionCode code = ReplaceChild(new_child, old_child);
    // Remove the transient reference but still keep the node if there is no
    // error.
    if (old_child)
      old_child->Unref(code == DOM_NO_ERR);
    return callbacks_->CheckException(code) ? old_child : NULL;
  }

  DOMNodeInterface *ScriptRemoveChild(DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Ref();
    DOMExceptionCode code = RemoveChild(old_child);
    // Remove the transient reference but still keep the node.
    if (old_child)
      old_child->Unref(code == DOM_NO_ERR);
    return callbacks_->CheckException(code) ? old_child : NULL;
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
      int ref_count = node_->GetRefCount();
      if (owner_node_) {
        // This node is detached from the old owner node.
        for (int i = 0; i < ref_count; i++)
          owner_node_->Unref();

        if (!new_owner) {
          // This node becomes a new orphan.
          if (node_->GetRefCount() == 0) {
            // This orphan is not referenced, delete it now.
            delete node_;
            return;
          } else {
            // This orphan is still referenced. Increase the document
            // orphan count.
            owner_document_->Ref();
          }
        }
      }

      if (new_owner) {
        // This node is attached to the new owner node.
        for (int i = 0; i < ref_count; i++)
          new_owner->Ref();

        if (!owner_node_) {
          // The node is no longer orphan root, so decrease the document
          // orphan count.
          owner_document_->Unref();
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
        : node_(node), children_(children) {
      node->Ref();
    }
    virtual ~ChildrenNodeList() {
      node_->Unref();
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
  std::string prefix_;
  std::string local_name_;
  DOMNodeInterface *parent_;
  // In most cases, owner_node_ == parent_, but for DOMAttr, owner_node_ is the
  // owner element.
  DOMNodeInterface *owner_node_;
  Children children_;
  DOMNodeImpl *previous_sibling_, *next_sibling_;
  std::string last_xml_;
  int row_, column_;
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
  using Super::SetInheritsFrom;
  using Super::SetDynamicPropertyHandler;
  using Super::SetArrayHandler;

  DOMNodeBase(DOMDocumentInterface *owner_document,
              const char *name)
      : impl_(new DOMNodeImpl(this, this, owner_document, name)) {
    SetInheritsFrom(GlobalNode::Get());
  }

  virtual void DoClassRegister() {
    // "baseName" is not in W3C standard. Register it to keep compatibility
    // with the Windows DOM.
    RegisterProperty("baseName", NewSlot(&DOMNodeBase::GetLocalName), NULL);
    RegisterProperty("localName", NewSlot(&DOMNodeBase::GetLocalName), NULL);
    RegisterProperty("nodeName", NewSlot(&DOMNodeBase::GetNodeName), NULL);
    RegisterProperty("nodeValue", NewSlot(&DOMNodeBase::GetNodeValue),
                     NewSlot(&DOMNodeBase::SetNodeValue));
    RegisterProperty("nodeType", NewSlot(&DOMNodeInterface::GetNodeType), NULL);
    // Don't register parentNode as a constant to prevent circular refs.
    RegisterProperty("parentNode",
                     NewSlot(&DOMNodeBase::GetParentNodeNotConst), NULL);
    RegisterProperty("childNodes",
                     NewSlot(&DOMNodeImpl::GetChildNodes, StaticGetImpl), NULL);
    RegisterProperty("firstChild",
                     NewSlot(&DOMNodeImpl::GetFirstChild, StaticGetImpl), NULL);
    RegisterProperty("lastChild",
                     NewSlot(&DOMNodeImpl::GetLastChild, StaticGetImpl), NULL);
    RegisterProperty("previousSibling",
                     NewSlot(&DOMNodeImpl::GetPreviousSibling, StaticGetImpl),
                     NULL);
    RegisterProperty("nextSibling",
                     NewSlot(&DOMNodeImpl::GetNextSibling, StaticGetImpl),
                     NULL);
    RegisterProperty("attributes", NewSlot(&DOMNodeBase::GetAttributesNotConst),
                     NULL);
    // Don't register ownerDocument as a constant to prevent circular refs.
    RegisterProperty("ownerDocument",
                     NewSlot(&DOMNodeBase::GetOwnerDocumentNotConst), NULL);
    RegisterProperty("prefix", NewSlot(&DOMNodeBase::GetPrefix),
                     NewSlot(&DOMNodeBase::SetPrefix));
    RegisterProperty("text", NewSlot(&DOMNodeBase::GetTextContent),
                     NewSlot(&DOMNodeBase::SetTextContent));
    RegisterMethod("insertBefore",
                   NewSlot(&DOMNodeImpl::ScriptInsertBefore, StaticGetImpl));
    RegisterMethod("replaceChild",
                   NewSlot(&DOMNodeImpl::ScriptReplaceChild, StaticGetImpl));
    RegisterMethod("removeChild",
                   NewSlot(&DOMNodeImpl::ScriptRemoveChild, StaticGetImpl));
    RegisterMethod("appendChild",
                   NewSlot(&DOMNodeImpl::ScriptAppendChild, StaticGetImpl));
    RegisterMethod("hasChildNodes",
                   NewSlot(&DOMNodeBase::HasChildNodes));
    // Register slot of this class to allow subclasses to override.
    RegisterMethod("cloneNode", NewSlot(&DOMNodeBase::CloneNode));
    RegisterMethod("normalize", NewSlot(&DOMNodeBase::Normalize));
  }

  virtual ~DOMNodeBase() {
    delete impl_;
    impl_ = NULL;
  }

  // Used in DoClassRegister().
  static DOMNodeImpl *StaticGetImpl(DOMNodeBase *node) { return node->impl_; }
  // Returns the address of impl_, to make it possible to access implementation
  // specific things through DOMNodeInterface pointers.
  virtual DOMNodeImpl *GetImpl() const { return impl_; }

  // Overrides ScriptableHelper::Ref().
  virtual void Ref() {
    if (impl_->owner_node_) {
      // Increase the reference count along the path to the root.
      impl_->owner_node_->Ref();
    }
    return Super::Ref();
  }
  // Overrides ScriptableHelper::Unref().
  virtual void Unref(bool transient = false) {
    if (impl_->owner_node_) {
      Super::Unref(true);
      // Decrease the reference count along the path to the root.
      impl_->owner_node_->Unref(transient);
    } else {
      // Only the root can delete the whole tree. Because the reference count
      // are accumulated, root's refcount == 0 means all decendents's
      // refcount == 0, thus the whole tree can be safely deleted.
      Super::Unref(transient);
    }
  }

  virtual std::string GetNodeName() const {
    return impl_->GetNodeName();
  }

  virtual const char *GetNodeValue() const { return NULL; }
  virtual void SetNodeValue(const char *node_value) { }
  // GetNodeType() is still abstract.

  virtual DOMNodeInterface *GetParentNode() { return impl_->parent_; }
  virtual const DOMNodeInterface *GetParentNode() const {
    return impl_->parent_;
  }
  DOMNodeInterface *GetParentNodeNotConst() { return GetParentNode(); }

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
  DOMNamedNodeMapInterface *GetAttributesNotConst() { return GetAttributes(); }

  virtual DOMDocumentInterface *GetOwnerDocument() {
    return impl_->owner_document_;
  }
  virtual const DOMDocumentInterface *GetOwnerDocument() const {
    return impl_->owner_document_;
  }
  DOMDocumentInterface *GetOwnerDocumentNotConst() {
    return GetOwnerDocument();
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

  DOMNodeListInterface *GetElementsByTagNameNotConst(const char *name) {
    return GetElementsByTagName(name);
  }

  virtual std::string GetTextContent() const {
    const char *content = GetNodeValue();
    return content ? content : impl_->GetChildrenTextContent();
  }

  virtual void SetTextContent(const char *text_content) {
    if (GetNodeValue())
      SetNodeValue(text_content);
    else
      impl_->SetChildTextContent(text_content);
  }

  virtual std::string GetXML() const {
    return impl_->GetXML();
  }

  virtual int GetRow() const {
    return impl_->row_;
  }

  virtual void SetRow(int row) {
    impl_->row_ = row;
  }

  virtual int GetColumn() const {
    return impl_->column_;
  }

  virtual void SetColumn(int column) {
    impl_->column_ = column;
  }

  virtual bool CheckException(DOMExceptionCode code) {
    return GlobalCheckException(this, code);
  }

  virtual const char *GetPrefix() const {
    return impl_->prefix_.empty() ? NULL : impl_->prefix_.c_str();
  }

  virtual DOMExceptionCode SetPrefix(const char *prefix) {
    return AllowPrefix() ? impl_->SetPrefix(prefix) : DOM_NO_ERR;
  }

  virtual std::string GetLocalName() const {
    return impl_->local_name_;
  }

  bool CheckXMLName(const char *name) const {
    return impl_->owner_document_->GetXMLParser()->CheckXMLName(name);
  }

  std::string EncodeXMLString(const char *xml) const {
    return impl_->owner_document_->GetXMLParser()->EncodeXMLString(xml);
  }

 protected:
  virtual bool AllowPrefix() const { return false; }

 private:
  DOMNodeImpl *impl_;
};

static const UTF16Char kBlankUTF16Str[] = { 0 };

template <typename Interface1>
class DOMCharacterData : public DOMNodeBase<Interface1> {
 public:
  typedef DOMNodeBase<Interface1> Super;
  DOMCharacterData(DOMDocumentInterface *owner_document,
                   const char *name, const UTF16Char *data)
      : Super(owner_document, name),
        data_(data ? data : kBlankUTF16Str) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("data", NewSlot(&DOMCharacterData::GetData),
                     NewSlot(&DOMCharacterData::SetData));
    RegisterProperty("length", NewSlot(&DOMCharacterData::GetLength), NULL);
    RegisterMethod("substringData",
                   NewSlot(&DOMCharacterData::ScriptSubstringData));
    RegisterMethod("appendData", NewSlot(&DOMCharacterData::AppendData));
    RegisterMethod("insertData", NewSlot(&DOMCharacterData::ScriptInsertData));
    RegisterMethod("deleteData", NewSlot(&DOMCharacterData::ScriptDeleteData));
    RegisterMethod("replaceData",
                   NewSlot(&DOMCharacterData::ScriptReplaceData));
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

  virtual UTF16String GetData() const { return data_; }
  virtual void SetData(const UTF16Char *data) {
    data_ = data ? data : kBlankUTF16Str;
    utf8_data_.clear();
  }
  virtual size_t GetLength() const { return data_.length(); }

  virtual DOMExceptionCode SubstringData(size_t offset, size_t count,
                                         UTF16String *result) const {
    ASSERT(result);
    result->clear();
    if (offset > data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(data_.length() - offset, count);
    *result = data_.substr(offset, count);
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
    UTF16String result;
    CheckException(SubstringData(offset, count, &result));
    return result;
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
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("name", NewSlot(&DOMAttr::GetName), NULL);
    // Our DOMAttrs are always specified, because we don't support DTD for now.
    RegisterConstant("specified", true);
    RegisterProperty("value", NewSlot(&DOMAttr::GetValue),
                     NewSlot(&DOMAttr::SetValue));
    // ownerElement is a DOM2 property, so not registered for now.
  }

  virtual DOMNodeInterface *CloneNode(bool deep) const {
    // Attr.cloneNode always clone its children, even if deep is false.
    return Super::CloneNode(true);
  }

  virtual const char *GetNodeValue() const {
    last_node_value_ = GetImpl()->GetChildrenTextContent();
    return last_node_value_.c_str();
  }
  virtual void SetNodeValue(const char *value) {
    GetImpl()->SetChildTextContent(value);
  }
  virtual NodeType GetNodeType() const { return ATTRIBUTE_NODE; }

  virtual std::string GetName() const { return GetNodeName(); }
  // Our DOMAttrs are always specified, because we don't support DTD for now.
  virtual bool IsSpecified() const { return true; }
  virtual std::string GetValue() const { return GetNodeValue(); }
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
    return new DOMAttr(GetOwnerDocument(), GetName().c_str(), NULL);
  }

  virtual bool AllowPrefix() const { return true; }

 private:
  DOMElement *owner_element_;
  mutable std::string last_node_value_;
};

class DOMElement : public DOMNodeBase<DOMElementInterface> {
 public:
  DEFINE_CLASS_ID(0x721f40f59a3f48a9, DOMElementInterface);
  typedef DOMNodeBase<DOMElementInterface> Super;
  typedef std::vector<DOMAttr *> Attrs;
  // Maps attribute name to the index of Attrs.
  typedef std::map<std::string, size_t> AttrsMap;

  DOMElement(DOMDocumentInterface *owner_document, const char *tag_name)
      : Super(owner_document, tag_name) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("tagName", NewSlot(&DOMElement::GetTagName), NULL);
    RegisterMethod("getAttribute", NewSlot(&DOMElement::GetAttribute));
    RegisterMethod("setAttribute",
                   NewSlot(&DOMElement::ScriptSetAttribute));
    RegisterMethod("removeAttribute",
                   NewSlot(&DOMElement::RemoveAttribute));
    RegisterMethod("getAttributeNode",
                   NewSlot(&DOMElement::GetAttributeNodeNotConst));
    RegisterMethod("setAttributeNode",
                   NewSlot(&DOMElement::ScriptSetAttributeNode));
    RegisterMethod("removeAttributeNode",
                   NewSlot(&DOMElement::ScriptRemoveAttributeNode));
    RegisterMethod("getElementsByTagName",
                   NewSlot(&Super::GetElementsByTagNameNotConst));
  }

  ~DOMElement() {
    ASSERT(attrs_.size() == attrs_map_.size());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      delete (*it);
  }

  virtual NodeType GetNodeType() const { return ELEMENT_NODE; }
  virtual std::string GetTagName() const { return GetNodeName(); }

  virtual void Normalize() {
    Super::Normalize();
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      (*it)->Normalize();
  }

  virtual std::string GetAttribute(const char *name) const {
    const DOMAttrInterface *attr = GetAttributeNode(name);
    // TODO: Default value logic.
    return attr ? attr->GetValue() : "";
  }

  virtual DOMExceptionCode SetAttribute(const char *name, const char *value) {
    if (!CheckXMLName(name))
      return DOM_INVALID_CHARACTER_ERR;

    AttrsMap::iterator it = attrs_map_.find(name);
    if (it == attrs_map_.end()) {
      DOMAttr *attr = new DOMAttr(GetOwnerDocument(), name, this);
      attrs_map_[attr->GetName()] = attrs_.size();
      attrs_.push_back(attr);
      attr->SetValue(value);
      attr->SetRow(GetRow());
      // Don't set column, because it is inaccurate.
      ASSERT(attrs_map_.size() == attrs_.size());
    } else {
      ASSERT(it->second < attrs_.size());
      attrs_[it->second]->SetValue(value);
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
    AttrsMap::const_iterator it = attrs_map_.find(name);
    ASSERT(it == attrs_map_.end() || it->second < attrs_.size());
    return it == attrs_map_.end() ? NULL : attrs_[it->second];
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

    DOMAttr *new_attr_internal = down_cast<DOMAttr *>(new_attr);
    new_attr_internal->SetOwnerElement(this);
    AttrsMap::iterator it = attrs_map_.find(new_attr->GetName());
    if (it != attrs_map_.end()) {
      ASSERT(it->second < attrs_.size());
      attrs_[it->second]->SetOwnerElement(NULL);
      attrs_[it->second] = new_attr_internal;
      // No need to change attr_map_.
    } else {
      attrs_map_[new_attr->GetName()] = attrs_.size();
      attrs_.push_back(new_attr_internal);
      ASSERT(attrs_map_.size() == attrs_.size());
    }
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode RemoveAttributeNode(DOMAttrInterface *old_attr) {
    if (!old_attr)
      return DOM_NULL_POINTER_ERR;
    if (old_attr->GetOwnerElement() != this)
      return DOM_NOT_FOUND_ERR;

    bool result = RemoveAttributeInternal(old_attr->GetName());
    ASSERT(result);
    return result ? DOM_NO_ERR : DOM_NOT_FOUND_ERR;
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
    DOMElement *element = new DOMElement(GetOwnerDocument(),
                                         GetTagName().c_str());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
      DOMAttrInterface *cloned_attr =
          down_cast<DOMAttrInterface *>((*it)->CloneNode(true));
      element->SetAttributeNode(cloned_attr);
    }
    return element;
  }

  virtual bool AllowPrefix() const { return true; }

 private:
  DOMAttrInterface *GetAttributeNodeNotConst(const char *name) {
    return GetAttributeNode(name);
  }

  class AttrsNamedMap : public ScriptableHelper<DOMNamedNodeMapInterface> {
   public:
    DEFINE_CLASS_ID(0xbe2998ee79754343, DOMNamedNodeMapInterface)
    AttrsNamedMap(DOMElement *element) : element_(element) {
      element->Ref();
      SetArrayHandler(NewSlot(this, &AttrsNamedMap::GetItemNotConst), NULL);
    }
    virtual ~AttrsNamedMap() {
      element_->Unref();
    }

    virtual void DoClassRegister() {
      RegisterProperty("length", NewSlot(&AttrsNamedMap::GetLength), NULL);
      RegisterMethod("getNamedItem",
                     NewSlot(&AttrsNamedMap::GetNamedItemNotConst));
      RegisterMethod("setNamedItem",
                     NewSlot(&AttrsNamedMap::ScriptSetNamedItem));
      RegisterMethod("removeNamedItem",
                     NewSlot(&AttrsNamedMap::ScriptRemoveNamedItem));
      RegisterMethod("item", NewSlot(&AttrsNamedMap::GetItemNotConst));
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
    DOMNodeInterface *GetItemNotConst(size_t index) { return GetItem(index); }
    DOMNodeInterface *GetNamedItemNotConst(const char *name) {
      return GetNamedItem(name);
    }

    DOMNodeInterface *ScriptSetNamedItem(DOMNodeInterface *arg) {
      if (!arg) {
        GlobalCheckException(this, DOM_NULL_POINTER_ERR);
        return NULL;
      } else if (arg->GetNodeType() != ATTRIBUTE_NODE) {
        GlobalCheckException(this, DOM_HIERARCHY_REQUEST_ERR);
        return NULL;
      } else {
        DOMAttrInterface *new_attr = down_cast<DOMAttrInterface *>(arg);
        DOMAttrInterface *replaced_attr = NULL;
        if (arg) {
          // Add a temporary reference to the replaced attr to prevent it from
          // being deleted in SetAttributeNode().
          replaced_attr = element_->GetAttributeNode(
              new_attr->GetName().c_str());
          if (replaced_attr)
            replaced_attr->Ref();
        }
        DOMExceptionCode code = element_->SetAttributeNode(new_attr);
        if (replaced_attr)
          replaced_attr->Unref(code == DOM_NO_ERR);
        return GlobalCheckException(this, code) ? replaced_attr : NULL;
      }
    }

    DOMNodeInterface *ScriptRemoveNamedItem(const char *name) {
      DOMNodeInterface *removed_node = GetNamedItem(name);
      if (removed_node)
        removed_node->Ref();
      DOMExceptionCode code = RemoveNamedItem(name);
      if (removed_node)
        removed_node->Unref(code == DOM_NO_ERR);
      return GlobalCheckException(this, code) ? removed_node : NULL;
    }

    DOMElement *element_;
  };

  void ScriptSetAttribute(const char *name, const char *value) {
    CheckException(SetAttribute(name, value));
  }

  DOMAttrInterface *ScriptSetAttributeNode(DOMAttrInterface *new_attr) {
    DOMAttrInterface *replaced_attr = NULL;
    if (new_attr) {
      replaced_attr = GetAttributeNode(new_attr->GetName().c_str());
      // Add a temporary reference to the replaced attr to prevent it from
      // being deleted in SetAttributeNode().
      if (replaced_attr)
        replaced_attr->Ref();
    }
    DOMExceptionCode code = SetAttributeNode(new_attr);
    if (replaced_attr)
      replaced_attr->Unref(code == DOM_NO_ERR);
    return CheckException(code) ? replaced_attr : NULL;
  }

  DOMAttrInterface *ScriptRemoveAttributeNode(DOMAttrInterface *old_attr) {
    return CheckException(RemoveAttributeNode(old_attr)) ? old_attr : NULL;
  }

  bool RemoveAttributeInternal(const std::string &name) {
    AttrsMap::iterator it = attrs_map_.find(name);
    if (it != attrs_map_.end()) {
      size_t index = it->second;
      attrs_[index]->SetOwnerElement(NULL);
      if (index < attrs_.size() - 1) {
        // Move the last element to the new blank slot and update the index,
        // ensuring that attrs_ contains no blank slot.
        DOMAttr *last_attr = attrs_.back();
        attrs_[index] = last_attr;
        attrs_map_[last_attr->GetName()] = index;
      }
      attrs_.pop_back();
      attrs_map_.erase(it);
      return true;
    }
    return false;
    // TODO: Deal with default values if we support DTD.
  }

  std::string tag_name_;
  Attrs attrs_;
  AttrsMap attrs_map_;
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
  UTF16String tail_data;
  text->SubstringData(offset, tail_size, &tail_data);
  *new_text = down_cast<DOMTextInterface *>(text->CloneNode(false));
  (*new_text)->SetData(tail_data.c_str());
  text->DeleteData(offset, tail_size);

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
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterMethod("splitText", NewSlot(&DOMText::ScriptSplitText));
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
    return new DOMText(GetOwnerDocument(), GetData().c_str());
  }

 private:
  DOMTextInterface *ScriptSplitText(size_t offset) {
    DOMTextInterface *result = NULL;
    return CheckException(SplitText(offset, &result)) ? result : NULL;
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
    return new DOMComment(GetOwnerDocument(), GetData().c_str());
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
    return new DOMCDATASection(GetOwnerDocument(), GetData().c_str());
  }
};

class DOMDocumentFragment : public DOMNodeBase<DOMDocumentFragmentInterface> {
 public:
  DEFINE_CLASS_ID(0x6ba54beef94643d4, DOMDocumentFragmentInterface);
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
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("target",
                     NewSlot(&DOMProcessingInstruction::GetTarget), NULL);
    RegisterProperty("data", NewSlot(&DOMProcessingInstruction::GetData),
                     NewSlot(&DOMProcessingInstruction::SetData));
  }

  virtual const char *GetNodeValue() const { return data_.c_str(); }
  virtual void SetNodeValue(const char *value) { SetData(value); }
  virtual NodeType GetNodeType() const { return PROCESSING_INSTRUCTION_NODE; }

  virtual std::string GetTarget() const { return target_; }
  virtual std::string GetData() const { return data_; }
  virtual void SetData(const char *data) { data_ = data ? data : ""; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    AppendIndentNewLine(indent, xml);
    xml->append("<?");
    xml->append(GetNodeName());
    xml->append(1, ' ');
    xml->append(GetData());
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

class DOMImplementation
    : public ScriptableHelperNativeOwned<DOMImplementationInterface> {
 public:
  DEFINE_CLASS_ID(0xd23149a89cf24e12, DOMImplementationInterface);

  virtual void DoClassRegister() {
    RegisterMethod("hasFeature", NewSlot(&DOMImplementation::HasFeature));
  }

  virtual bool HasFeature(const char *feature, const char *version) const {
    return feature && strcasecmp(feature, "XML") == 0 &&
          (!version || !version[0] || strcmp(version, "1.0") == 0);
  }
};

// This class is not a complete implementation, just to make some script
// containing Microsoft-specific code run without errors.
class ParseError : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xc494c55756dc46a6, ScriptableInterface);
  ParseError() : code_(0) {
    RegisterReadonlySimpleProperty("errorCode", &code_);
    RegisterConstant("filepos", 0);
    RegisterConstant("line", 0);
    RegisterConstant("linepos", 0);
    RegisterConstant("reason", "");
    RegisterConstant("srcText", "");
    RegisterConstant("url", "");
  }
  void SetCode(int code) { code_ = code; }
 private:
  int code_;
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

  DOMDocument(XMLParserInterface *xml_parser)
      : Super(NULL, kDOMDocumentName),
        xml_parser_(xml_parser) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterConstant("doctype", static_cast<ScriptableInterface *>(NULL));
    RegisterConstant("implementation", &dom_implementation_);
    RegisterProperty("documentElement",
                     NewSlot(&DOMDocument::GetDocumentElementNotConst), NULL);
    RegisterMethod("loadXML", NewSlot(&DOMDocument::LoadXML));
    RegisterMethod("createElement",
                   NewSlot(&DOMDocument::ScriptCreateElement));
    RegisterMethod("createDocumentFragment",
                   NewSlot(&DOMDocument::CreateDocumentFragment));
    RegisterMethod("createTextNode",
                   NewSlot(&DOMDocument::CreateTextNode));
    RegisterMethod("createComment", NewSlot(&DOMDocument::CreateComment));
    RegisterMethod("createCDATASection",
                   NewSlot(&DOMDocument::CreateCDATASection));
    RegisterMethod("createProcessingInstruction",
                   NewSlot(&DOMDocument::ScriptCreateProcessingInstruction));
    RegisterMethod("createAttribute",
                   NewSlot(&DOMDocument::ScriptCreateAttribute));
    RegisterMethod("createEntityReference",
                   NewSlot(&DOMDocument::ScriptCreateEntityReference));
    RegisterMethod("getElementsByTagName",
                   NewSlot(&Super::GetElementsByTagNameNotConst));
    // Compatibility with Microsoft DOM.
    RegisterProperty("async", NULL, NewSlot(&DummySetter));
    RegisterProperty("parseError", NewSlot(&DOMDocument::GetParseError), NULL);
  }

  ParseError *GetParseError() {
    return &parse_error_;
  }

  virtual bool LoadXML(const char *xml) {
    GetImpl()->RemoveAllChildren();
    bool result = xml_parser_->ParseContentIntoDOM(xml, NULL, "NONAME", NULL,
                                                   NULL, kEncodingFallback,
                                                   this, NULL, NULL);
    parse_error_.SetCode(result ? 0 : 1);
    return result;
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
    return &dom_implementation_;
  }
  virtual const DOMImplementationInterface *GetImplementation() const {
    return &dom_implementation_;
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
    if (!xml_parser_->CheckXMLName(tag_name))
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
    if (!xml_parser_->CheckXMLName(target))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMProcessingInstruction(this, target, data);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode CreateAttribute(const char *name,
                                           DOMAttrInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!xml_parser_->CheckXMLName(name))
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

  virtual XMLParserInterface *GetXMLParser() const {
    return xml_parser_;
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
  DOMElementInterface *GetDocumentElementNotConst() {
    return GetDocumentElement();
  }

  const DOMNodeInterface *FindNodeOfType(NodeType type) const {
    const DOMNodeInterface *result = NULL;
    for (const DOMNodeInterface *item = GetFirstChild(); item;
         item = item->GetNextSibling()) {
      if (item->GetNodeType() == type) {
        result = item;
        break;
      }
    }
    return result;
  }

  DOMElementInterface *ScriptCreateElement(const char *tag_name) {
    DOMElementInterface *result = NULL;
    return CheckException(CreateElement(tag_name, &result)) ? result : NULL;
  }

  DOMProcessingInstructionInterface *ScriptCreateProcessingInstruction(
      const char *target, const char *data) {
    DOMProcessingInstructionInterface *result = NULL;
    return CheckException(CreateProcessingInstruction(target, data, &result)) ?
           result : NULL;
  }

  DOMAttrInterface *ScriptCreateAttribute(const char *name) {
    DOMAttrInterface *result = NULL;
    return CheckException(CreateAttribute(name, &result)) ? result : NULL;
  }

  ScriptableInterface *ScriptCreateEntityReference(const char *name) {
    // TODO: if we support DTD.
    return NULL;
  }

  XMLParserInterface *xml_parser_;
  static DOMImplementation dom_implementation_;
  ParseError parse_error_;
};

DOMImplementation DOMDocument::dom_implementation_;

} // namespace internal

DOMDocumentInterface *CreateDOMDocument(XMLParserInterface *xml_parser) {
  ASSERT(xml_parser);
  return new ::ggadget::internal::DOMDocument(xml_parser);
}

} // namespace ggadget
