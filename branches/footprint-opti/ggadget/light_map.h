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

// Part of this file (LokiAllocator) is from the Loki Library.
////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

#ifndef GGADGET_LIGHT_MAP_H__
#define GGADGET_LIGHT_MAP_H__

#include <map>
#include <set>
#include <ggadget/common.h>
#include <ggadget/small_object.h>

namespace ggadget {

// The following code is from Loki(0.1.7)'s Allocator.h
//-----------------------------------------------------------------------------

/** @class LokiAllocator
 Adapts Loki's Small-Object Allocator for STL container classes.
 This class provides all the functionality required for STL allocators, but
 uses Loki's Small-Object Allocator to perform actual memory operations.
 Implementation comes from a post in Loki forums (by Rasmus Ekman?).
 */
template
<
    typename Type,
    typename AllocT = AllocatorSingleton<>
>
class LokiAllocator
{
public:

    typedef ::std::size_t size_type;
    typedef ::std::ptrdiff_t difference_type;
    typedef Type * pointer;
    typedef const Type * const_pointer;
    typedef Type & reference;
    typedef const Type & const_reference;
    typedef Type value_type;

    /// Default constructor does nothing.
    inline LokiAllocator( void ) throw() { }

    /// Copy constructor does nothing.
    inline LokiAllocator( const LokiAllocator & ) throw() { }

    /// Type converting allocator constructor does nothing.
    template < typename Type1 >
    inline LokiAllocator( const LokiAllocator< Type1 > & ) throw() { }

    /// Destructor does nothing.
    inline ~LokiAllocator() throw() { }

    /// Convert an allocator<Type> to an allocator <Type1>.
    template < typename Type1 >
    struct rebind
    {
        typedef LokiAllocator< Type1 > other;
    };

    /// Return address of reference to mutable element.
    pointer address( reference elem ) const { return &elem; }

    /// Return address of reference to const element.
    const_pointer address( const_reference elem ) const { return &elem; }

    /** Allocate an array of count elements.  Warning!  The true parameter in
     the call to Allocate means this function can throw exceptions.  This is
     better than not throwing, and returning a null pointer in case the caller
     assumes the return value is not null.
     @param count # of elements in array.
     @param hint Place where caller thinks allocation should occur.
     @return Pointer to block of memory.
     */
    pointer allocate( size_type count, const void * hint = 0 )
    {
        (void)hint;  // Ignore the hint.
        void * p = AllocT::Instance().Allocate( count * sizeof( Type ), true );
        return reinterpret_cast< pointer >( p );
    }
    /// Ask allocator to release memory at pointer with size bytes.
    void deallocate( pointer p, size_type size )
    {
        AllocT::Instance().Deallocate( p, size * sizeof( Type ) );
    }

    /// Calculate max # of elements allocator can handle.
    size_type max_size( void ) const throw()
    {
        // A good optimizer will see these calculations always produce the same
        // value and optimize this function away completely.
        const size_type max_bytes = size_type( -1 );
        const size_type bytes = max_bytes / sizeof( Type );
        return bytes;
    }

    /// Construct an element at the pointer.
    void construct( pointer p, const Type & value )
    {
        // A call to global placement new forces a call to copy constructor.
        ::new( p ) Type( value );
    }

    /// Destruct the object at pointer.
    void destroy( pointer p )
    {
        // If the Type has no destructor, then some compilers complain about
        // an unreferenced parameter, so use the void cast trick to prevent
        // spurious warnings.
        (void)p;
        p->~Type();
    }

};

//-----------------------------------------------------------------------------

/** All equality operators return true since LokiAllocator is basically a
 monostate design pattern, so all instances of it are identical.
 */
template < typename Type >
inline bool operator == ( const LokiAllocator< Type > &, const LokiAllocator< Type > & )
{
    return true;
}

/** All inequality operators return false since LokiAllocator is basically a
 monostate design pattern, so all instances of it are identical.
 */
template < typename Type >
inline bool operator != ( const LokiAllocator< Type > & , const LokiAllocator< Type > & )
{
    return false;
}

//-----------------------------------------------------------------------------
// GGL code below.

#define _LIGHT_MAP_DO_CAST true

// Any type that is not specialized later will be casted to itself.
template <typename Type> struct _Cast {
  typedef Type T;
  static const bool do_cast = false;
  static inline const T &From(const T &v) { return v; }
  static inline const T &To(const T &v) { return v; }
};
template <typename Type> struct _Cast<Type *> {
  typedef void *T;
  typedef Type *ValueT;
  static const bool do_cast = _LIGHT_MAP_DO_CAST;
  static inline const T &From(const ValueT &v)
  { return reinterpret_cast<const T &>(v); }
  static inline const ValueT &To(const T &v)
  { return reinterpret_cast<const ValueT &>(v); }
};
template <typename Type> struct _Cast<const Type *> {
  typedef void *T;
  typedef const Type *ValueT;
  typedef Type *ValueTNoConst;
  static const bool do_cast = _LIGHT_MAP_DO_CAST;
  static inline const T &From(const ValueT &v)
  { return reinterpret_cast<const T &>(const_cast<const ValueTNoConst &>(v)); }
  static inline const ValueT &To(const T &v)
  { return reinterpret_cast<const ValueT &>(v); }
};

// Any integral types (except enum types which are not supported for now)
// can be cast to "void *".
#define _DEFINE_CAST_TYPE(type)                   \
template <> struct _Cast<type> {                  \
  typedef void *T;                                \
  static const bool do_cast = _LIGHT_MAP_DO_CAST; \
  static inline const T &From(const type &v)      \
  { return reinterpret_cast<const T &>(v); }      \
  static inline const type &To(const T &v)        \
  { return reinterpret_cast<const type &>(v); }   \
};
_DEFINE_CAST_TYPE(bool)
_DEFINE_CAST_TYPE(char)
_DEFINE_CAST_TYPE(unsigned char)
_DEFINE_CAST_TYPE(short)
_DEFINE_CAST_TYPE(unsigned short)
_DEFINE_CAST_TYPE(int)
_DEFINE_CAST_TYPE(unsigned int)
_DEFINE_CAST_TYPE(long)
_DEFINE_CAST_TYPE(unsigned long)
#if GGL_SIZEOF_LONG_INT != 8
_DEFINE_CAST_TYPE(int64_t)
_DEFINE_CAST_TYPE(uint64_t)
#endif
#undef _DEFINE_CAST_TYPE

// Makes it shorter.
#define _CAST(t) typename _Cast<t>::T

template <typename Value, typename Compare, bool DoCast = _Cast<Value>::do_cast>
class CastCompare {
 public:
  COMPILE_ASSERT(DoCast, Or_should_be_handled_in_the_specialized_version);
  CastCompare(Compare comp) : comp_(comp) { }
  bool operator()(const _CAST(Value) &x, const _CAST(Value) &y) const
  { return comp_(_Cast<Value>::To(x), _Cast<Value>::To(y)); }
 private:
  Compare comp_;
};

template <typename Value, typename Compare>
class CastCompare<Value, Compare, false> : public Compare {
 public:
  CastCompare() { }
  CastCompare(Compare comp) { }
};

/**
 * The base type shared by LightMap and LightMultiMap because most of the
 * methods of std::map and std::multimap have the same signatures.
 */
template <typename Key, typename Value, typename Compare, typename Super>
class LightMapBase : public Super {
 public:
  typedef typename Super::mapped_type super_mapped_type;
  typedef typename Super::key_type super_key_type;
  typedef typename Super::value_type super_value_type;
  typedef typename Super::iterator super_iterator;
  typedef typename Super::const_iterator super_const_iterator;
  typedef typename Super::reverse_iterator super_reverse_iterator;
  typedef typename Super::const_reverse_iterator super_const_reverse_iterator;
  typedef typename Super::size_type size_type;
  typedef typename Super::difference_type difference_type;
  typedef typename Super::key_compare super_key_compare;

  // We must return a temporary pair for iterator's '->' operator because the
  // fields in the pair are casted, so the pair can't be a reference to its
  // original place and thus must be const to avoid misuse.
  typedef std::pair<Key, Value> iterator_value_type;

  template <typename Iterator>
  struct CastIterator {
    typedef iterator_value_type value_type;
    typedef CastIterator<Iterator> Self;
    CastIterator() { }
    explicit CastIterator(const Iterator &it) : it_(it) { }

    // Any another CastIterator whose it_ is assignable to this.it_ should be
    // able to be converted automatically into this class. This occurs when
    // const_iterator is expected for a non-const method invocation.
    template <typename CastIterator1>
    CastIterator(const CastIterator1 &another) : it_(another.it_) { }

    // Don't provide operator* because we have no idea how to implement it.
    const value_type *operator->() const
    { return &(LightMapBase::temp_iterator_value_ = value_type(
          // Here remove the constness of the key temporarily so that we can
          // store it into the temporary value. The constant correctness is
          // ensured by the return type of this function.
          _Cast<Key>::To(const_cast<super_key_type &>(it_->first)),
          _Cast<Value>::To(const_cast<super_mapped_type &>(it_->second)))); }
    Self &operator++() { ++it_; return *this; }
    Self operator++(int) { Self tmp = *this; ++it_; return tmp; }
    Self &operator--() { --it_; return *this; }
    Self operator--(int) { Self tmp = *this; --it_; return tmp; }
    template <typename CastIterator1>
    bool operator==(const CastIterator1 &i) const { return i.it_ == it_; }
    template <typename CastIterator1>
    bool operator!=(const CastIterator1 &i) const { return i.it_ != it_; }
    Iterator it_;
  };

  typedef Key key_type;
  typedef Value mapped_type;
  typedef Compare key_compare;
  typedef std::pair<const Key, Value> value_type;
  typedef CastIterator<super_iterator> iterator;
  typedef CastIterator<super_const_iterator> const_iterator;
  typedef CastIterator<super_reverse_iterator> reverse_iterator;
  typedef CastIterator<super_const_reverse_iterator> const_reverse_iterator;

  LightMapBase() : Super(super_key_compare(Compare())) { }
  LightMapBase(const Compare &comp) : Super(super_key_compare(comp)) { }
  LightMapBase(const LightMapBase &x) : Super(x) { }
  template <typename I> LightMapBase(I first, I last) : Super(first, last) { }
  LightMapBase &operator=(const LightMapBase &x)
  { Super::operator=(x); return *this; }

  iterator begin() { return iterator(Super::begin()); }
  const_iterator begin() const { return const_iterator(Super::begin()); }
  iterator end() { return iterator(Super::end()); }
  const_iterator end() const { return const_iterator(Super::end()); }
  reverse_iterator rbegin() { return reverse_iterator(Super::rbegin()); }
  const_reverse_iterator rbegin() const
  { return const_reverse_iterator(Super::rbegin()); }
  reverse_iterator rend() { return reverse_iterator(Super::rend()); }
  const_reverse_iterator rend() const
  { return const_reverse_iterator(Super::rend()); }
  Value &operator[](const Key &k)
  { return reinterpret_cast<Value &>(Super::operator[](_Cast<Key>::From(k))); }
  iterator insert(iterator i, const value_type &x)
  { return iterator(Super::insert(i.it_, super_value_type(
        _Cast<Key>::From(x.first), _Cast<Value>::From(x.second)))); }
  void erase(iterator i) { Super::erase(i.it_); }
  void erase(iterator a, iterator b) { Super::erase(a.it_, b.it_); }
  size_type erase(const Key &k) { return Super::erase(_Cast<Key>::From(k)); }
  iterator find(const Key &k)
  { return iterator(Super::find(_Cast<Key>::From(k))); }
  const_iterator find(const Key &k) const
  { return const_iterator(Super::find(_Cast<Key>::From(k))); }
  size_t count(const Key &k) const { return Super::count(_Cast<Key>::From(k)); }
  iterator lower_bound(const Key &k)
  { return iterator(Super::lower_bound(_Cast<Key>::From(k))); }
  const_iterator lower_bound(const Key &k) const
  { return const_iterator(Super::lower_bound(_Cast<Key>::From(k))); }
  iterator upper_bound(const Key &k)
  { return iterator(Super::upper_bound(_Cast<Key>::From(k))); }
  const_iterator upper_bound(const Key &k) const
  { return const_iterator(Super::upper_bound(_Cast<Key>::From(k))); }
  std::pair<iterator, iterator> equal_range(const Key &k)
  { std::pair<super_iterator, super_iterator> r =
        Super::equal_range(_Cast<Key>::From(k));
    return std::make_pair(iterator(r.first), iterator(r.second)); }
  std::pair<const_iterator, const_iterator> equal_range(const Key &k) const
  { std::pair<super_const_iterator, super_const_iterator> r =
        Super::equal_range(_Cast<Key>::From(k));
    return std::make_pair(const_iterator(r.first), const_iterator(r.second)); }

 private:
  // The temporary space to store the result of iterator's operator->().
  static std::pair<Key, Value> temp_iterator_value_;
};

template <typename Key, typename Value, typename Compare, typename Super>
std::pair<Key, Value>
    LightMapBase<Key, Value, Compare, Super>::temp_iterator_value_;

/**
 * std::map using @c LokiAllocator and possibly sharing base classes.
 * If the keys or values can be safely reinterpret casted void *, they
 * will share the common base class as much as possible to reduce code size.
 */
template <typename Key, typename Value, typename Compare = std::less<Key>,
          bool DoCast = _Cast<Value>::do_cast>
class LightMap : public LightMapBase<Key, Value, Compare,
    std::map<_CAST(Key), _CAST(Value), CastCompare<Key, Compare>,
             LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > > {
 public:
  COMPILE_ASSERT(DoCast, Or_should_be_handled_in_the_specialized_version);
  typedef std::map<_CAST(Key), _CAST(Value), CastCompare<Key, Compare>,
      LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > SuperSuper;
  typedef LightMapBase<Key, Value, Compare, SuperSuper> Super;
  LightMap() { }
  LightMap(const Compare &comp) : Super(comp) { }
  LightMap(const LightMap &x) : Super(x) { }
  template <typename I> LightMap(I first, I last) : Super(first, last) { }
  LightMap &operator=(const LightMap &x)
  { Super::operator=(x); return *this; }

  std::pair<typename Super::iterator, bool> insert(
      const typename Super::value_type &x)
  { std::pair<typename Super::super_iterator, bool> r = SuperSuper::insert(
        typename Super::super_value_type(_Cast<Key>::From(x.first),
                                         _Cast<Value>::From(x.second)));
    return std::make_pair(typename Super::iterator(r.first), r.second); }
};

/**
 * Specialized version of LightMap for maps whose values are castable to
 * void *, but haven't a customized comparator.
 * Note: This specialization uses std::less<_CAST(Key)> (might be
 * std::less<void *>) might change the order of the values in the map.
 */
template <typename Key, typename Value>
class LightMap<Key, Value, std::less<Key>, true>
    : public LightMapBase<Key, Value, std::less<_CAST(Key)>,
         std::map<_CAST(Key), _CAST(Value), std::less<_CAST(Key)>,
             LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > > {
 public:
  typedef std::map<_CAST(Key), _CAST(Value), std::less<_CAST(Key)>,
      LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > SuperSuper;
  typedef LightMapBase<Key, Value, std::less<_CAST(Key)>, SuperSuper> Super;
  LightMap() { }
  LightMap(const LightMap &x) : Super(x) { }
  template <typename I> LightMap(I first, I last) : Super(first, last) { }
  LightMap &operator=(const LightMap &x)
  { Super::operator=(x); return *this; }

  std::pair<typename Super::iterator, bool> insert(
      const typename Super::value_type &x)
  { std::pair<typename Super::super_iterator, bool> r = SuperSuper::insert(
        typename Super::super_value_type(_Cast<Key>::From(x.first),
                                         _Cast<Value>::From(x.second)));
    return std::make_pair(typename Super::iterator(r.first), r.second); }
};

/**
 * Specialized version of LightMap for maps whose keys and values are all
 * not castable to void *.
 */
template <typename Key, typename Value, typename Compare>
class LightMap<Key, Value, Compare, false>
    : public std::map<Key, Value, Compare,
                      LokiAllocator<std::pair<const Key, Value> > > {
 public:
  typedef std::map<Key, Value, Compare,
                   LokiAllocator<std::pair<const Key, Value> > > Super;
  LightMap() { }
  LightMap(const Compare &comp) : Super(comp) { }
  LightMap(const LightMap &x) : Super(x) { }
  template <typename I> LightMap(I first, I last) : Super(first, last) { }
  LightMap &operator=(const LightMap &x)
  { Super::operator=(x); return *this; }
};

template <typename Key, typename Value, typename Compare = std::less<Key>,
          bool DoCast = _Cast<Value>::do_cast>
class LightMultiMap : public LightMapBase<Key, Value, Compare,
    std::multimap<_CAST(Key), _CAST(Value), CastCompare<Key, Compare>,
        LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > > {
 public:
  typedef std::multimap<_CAST(Key), _CAST(Value), CastCompare<Key, Compare>,
      LokiAllocator<std::pair<const _CAST(Key), _CAST(Value)> > > SuperSuper;
  typedef LightMapBase<Key, Value, Compare, SuperSuper> Super;
  LightMultiMap() { }
  LightMultiMap(const Compare &comp) : Super(comp) { }
  LightMultiMap(const LightMultiMap &x) : Super(x) { }
  template <typename I> LightMultiMap(I first, I last) : Super(first, last) { }
  LightMultiMap &operator=(const LightMultiMap &x)
  { Super::operator=(x); return *this; }

  typename Super::iterator insert(const typename Super::value_type &x)
  { return typename Super::iterator(SuperSuper::insert(
        typename Super::super_value_type(_Cast<Key>::From(x.first),
                                         _Cast<Value>::From(x.second)))); }
};

template <typename Key, typename Value, typename Compare>
class LightMultiMap<Key, Value, Compare, false>
    : public std::multimap<Key, Value, Compare,
                           LokiAllocator<std::pair<const Key, Value> > > {
 public:
  typedef std::multimap<Key, Value, Compare,
                        LokiAllocator<std::pair<const Key, Value> > > Super;
  LightMultiMap() { }
  LightMultiMap(const Compare &comp) : Super(comp) { }
  LightMultiMap(const LightMultiMap &x) : Super(x) { }
  template <typename I> LightMultiMap(I first, I last) : Super(first, last) { }
  LightMultiMap &operator=(const LightMultiMap &x)
  { Super::operator=(x); return *this; }
};

/**
 * std::set using @c LokiAllocator and possibly sharing base classes.
 * If the keys can be safely reinterpret casted void *, they will share
 * the common base class to reduce code size.
 */
template <typename Key, typename Compare = std::less<Key>,
          bool DoCast = _Cast<Key>::do_cast>
class LightSet : public std::set<_CAST(Key), CastCompare<Key, Compare>,
                                 LokiAllocator<_CAST(Key)> > {
 public:
  typedef std::set<_CAST(Key), CastCompare<Key, Compare>,
                   LokiAllocator<_CAST(Key)> > Super;

  typedef typename Super::key_type super_key_type;
  typedef typename Super::iterator super_iterator;
  typedef typename Super::const_iterator super_const_iterator;
  typedef typename Super::reverse_iterator super_reverse_iterator;
  typedef typename Super::const_reverse_iterator super_const_reverse_iterator;
  typedef typename Super::size_type size_type;
  typedef typename Super::difference_type difference_type;
  typedef typename Super::key_compare super_key_compare;

  template <typename Iterator>
  struct CastIterator {
    typedef Key value_type;
    typedef CastIterator<Iterator> Self;
    CastIterator() { }
    explicit CastIterator(const Iterator &it) : it_(it) { }
    Key &operator*() { return reinterpret_cast<Key &>(
        const_cast<typename Iterator::value_type &>(it_.operator*())); }
    Key *operator->() { return reinterpret_cast<Key *>(
        const_cast<typename Iterator::value_type *>(it_.operator->())); }
    Self &operator++() { ++it_; return *this; }
    Self operator++(int) { Self tmp = *this; ++it_; return tmp; }
    Self &operator--() { --it_; return *this; }
    Self operator--(int) { Self tmp = *this; --it_; return tmp; }
    template <typename CastIterator1>
    bool operator==(const CastIterator1 &i) const { return i.it_ == it_; }
    template <typename CastIterator1>
    bool operator!=(const CastIterator1 &i) const { return i.it_ != it_; }
    Iterator it_;
  };

  template <typename Iterator>
  struct CastConstIterator {
    typedef Key value_type;
    typedef CastConstIterator<Iterator> Self;
    CastConstIterator() { }
    explicit CastConstIterator(Iterator it) : it_(it) { }
    template <typename CastIterator1>
    CastConstIterator(const CastIterator1 &another) : it_(another.it_) { }
    const Key &operator*() const { return _Cast<Key>::To(it_.operator*()); }
    const Key *operator->() const { return &_Cast<Key>::To(it_.operator*()); }
    Self &operator++() { ++it_; return *this; }
    Self operator++(int) { Self tmp = *this; ++it_; return tmp; }
    Self &operator--() { --it_; return *this; }
    Self operator--(int) { Self tmp = *this; --it_; return tmp; }
    template <typename CastIterator1>
    bool operator==(const CastIterator1 &i) const { return i.it_ == it_; }
    template <typename CastIterator1>
    bool operator!=(const CastIterator1 &i) const { return i.it_ != it_; }
    Iterator it_;
  };

  typedef Key key_type;
  typedef Key value_type;
  typedef Compare key_compare;
  typedef CastIterator<super_iterator> iterator;
  typedef CastConstIterator<super_const_iterator> const_iterator;
  typedef CastIterator<super_reverse_iterator> reverse_iterator;
  typedef CastConstIterator<super_const_reverse_iterator>
      const_reverse_iterator;

  LightSet() : Super(super_key_compare(Compare())) { }
  LightSet(const Compare &comp) : Super(super_key_compare(comp)) { }
  LightSet(const LightSet &x) : Super(x) { }
  template <typename I> LightSet(I first, I last) : Super(first, last) { }
  LightSet &operator=(const LightSet &x)
  { Super::operator=(x); return *this; }

  iterator begin() { return iterator(Super::begin()); }
  const_iterator begin() const { return const_iterator(Super::begin()); }
  iterator end() { return iterator(Super::end()); }
  const_iterator end() const { return const_iterator(Super::end()); }
  reverse_iterator rbegin() { return reverse_iterator(Super::rbegin()); }
  const_reverse_iterator rbegin() const
  { return const_reverse_iterator(Super::rbegin()); }
  reverse_iterator rend() { return reverse_iterator(Super::rend()); }
  const_reverse_iterator rend() const
  { return const_reverse_iterator(Super::rend()); }
  std::pair<iterator, bool> insert(const Key &x)
  { std::pair<super_iterator, bool> r = Super::insert(ToSuper(x));
    return std::make_pair(iterator(r.first), r.second); }
  iterator insert(iterator i, const Key &x)
  { return iterator(Super::insert(i.it_, ToSuper(x))); }
  void erase(iterator i) { Super::erase(i.it_); }
  void erase(iterator a, iterator b) { Super::erase(a.it_, b.it_); }
  size_type erase(const Key &k) { return Super::erase(_Cast<Key>::From(k)); }
  iterator find(const Key &k) { return iterator(Super::find(ToSuper(k))); }
  const_iterator find(const Key &k) const
  { return const_iterator(Super::find(ToSuper(k))); }
  size_t count(const Key &k) const { return Super::find(ToSuper(k)); }
  iterator lower_bound(const Key &k)
  { return iterator(Super::lower_bound(ToSuper(k))); }
  const_iterator lower_bound(const Key &k) const
  { return const_iterator(Super::lower_bound(ToSuper(k))); }
  iterator upper_bound(const Key &k)
  { return iterator(Super::upper_bound(ToSuper(k))); }
  const_iterator upper_bound(const Key &k) const
  { return const_iterator(Super::upper_bound(ToSuper(k))); }
  std::pair<iterator, iterator> equal_range(const Key &k)
  { std::pair<super_iterator, super_iterator> r =
        Super::equal_range(ToSuper(k));
    return std::make_pair(iterator(r.first), iterator(r.second)); }
  std::pair<const_iterator, const_iterator> equal_range(const Key &k) const
  { std::pair<super_const_iterator, super_const_iterator> r =
        Super::equal_range(ToSuper(k));
    return std::make_pair(const_iterator(r.first), const_iterator(r.second)); }

 private:
  inline static const super_key_type &ToSuper(const Key &k)
  { return reinterpret_cast<const super_key_type &>(k); }
  inline static const Key &ToSelf(const super_key_type &k)
  { return reinterpret_cast<const Key &>(k); }
};

/**
 * Specialized version of LightSet for sets whose keys are not castable to
 * void *.
 */
template <typename Key, typename Compare>
class LightSet<Key, Compare, false>
    : public std::set<Key, Compare, LokiAllocator<Key> > {
 public:
  typedef std::set<Key, Compare, LokiAllocator<Key> > Super;
  LightSet() { }
  LightSet(const Compare &comp) : Super(comp) { }
  LightSet(const LightSet &x) : Super(x) { }
  template <typename I> LightSet(I first, I last) : Super(first, last) { }
  LightSet &operator=(const LightSet &x)
  { Super::operator=(x); return *this; }
};

#undef _CAST

} // namespace ggadget

#endif // end file guardian
