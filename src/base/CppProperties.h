// Some utility templates for emulating properties --
// preferring a library solution to a new language feature
// Each property has three sets of redundant acccessors:
// 1. function call syntax
// 2. get() and set() functions
// 3. overloaded operator =
// a read-write property with data store and
// automatically generated get/set functions.
// this is what C++/CLI calls a trivial scalar property
template <class T>
class Property
{
  T data;
public:
// access with function call syntax
  Property() : data() { }
  T operator()() const
  {
    return data;
  }
  T operator()(T const & value)
  {
    data = value;
    return data;
  }
// access with get()/set() syntax
  T get() const
  {
    return data;
  }
  T set(T const & value)
  {
    data = value;
    return data;
  }
// access with '=' sign
// in an industrial-strength library,
// specializations for appropriate types might choose to
// add combined operators like +=, etc.
  operator T() const
  {
    return data;
  }
  T operator = (T const & value)
  {
    data = value;
    return data;
  }
  typedef T value_type; // might be useful for template deductions
};
// a read-only property calling a user-defined getter
template <class T, class Object, typename T(Object::*real_getter)()>
class ROProperty
{
  Object * my_object;
public:
// this function must be called by the containing class, normally in a
// constructor, to initialize the ROProperty so it knows where its
// real implementation code can be found. obj is usually the containing
// class, but need not be; it could be a special implementation object.
  void operator()(Object * obj)
  {
    my_object = obj;
  }
// function call syntax
  T operator()() const
  {
    return (my_object->*real_getter)();
  }
// get/set syntax
  T get() const
  {
    return (my_object->*real_getter)();
  }
  void set(T const & value);   // reserved but not implemented, per C++/CLI
// use on rhs of '='
  operator T() const
  {
    return (my_object->*real_getter)();
  }
  typedef T value_type; // might be useful for template deductions
};
// a write-only property calling a user-defined setter
template <
class T,
      class Object,
      typename T(Object::*real_setter)(T const &)
      >
class WOProperty
{
  Object * my_object;
public:
// this function must be called by the containing class, normally in
// a constructor, to initialize the WOProperty so it knows where its
// real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
// function call syntax
  T operator()(T const & value)
  {
    return (my_object->*real_setter)(value);
  }
// get/set syntax
  T get() const; // name reserved but not implemented per C++/CLI
  T set(T const & value)
  {
    return (my_object->*real_setter)(value);
  }
// access with '=' sign
  T operator = (T const & value)
  {
    return (my_object->*real_setter)(value);
  }
  typedef T value_type; // might be useful for template deductions
};
// a read-write property which invokes user-defined functions
template <
class T,
      class Object,
      typename T(Object::*real_getter)(),
      typename T(Object::*real_setter)(T const &)
      >
class RWProperty
{
  Object * my_object;
public:
// this function must be called by the containing class, normally in a
// constructor, to initialize the ROProperty so it knows where its
// real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
// function call syntax
  T operator()() const
  {
    return (my_object->*real_getter)();
  }
  T operator()(T const & value)
  {
    return (my_object->*real_setter)(value);
  }
// get/set syntax
  T get() const
  {
    return (my_object->*real_getter)();
  }
  T set(T const & value)
  {
    return (my_object->*real_setter)(value);
  }
// access with '=' sign
  operator T() const
  {
    return (my_object->*real_getter)();
  }
  T operator = (T const & value)
  {
    return (my_object->*real_setter)(value);
  }
  typedef T value_type; // might be useful for template deductions
};
// a read/write property providing indexed access.
// this class simply encapsulates a std::map and changes its interface
// to functions consistent with the other property<> classes.
// note that the interface combines certain limitations of std::map with
// some others from indexed properties as I understand them.
// an example of the first is that operator[] on a map will insert a
// key/value pair if it isn't already there. A consequence of this is that
// it can't be a const member function (and therefore you cannot access
// a const map using operator [].)
// an example of the second is that indexed properties do not appear
// to have any facility for erasing key/value pairs from the container.
// C++/CLI properties can have multi-dimensional indexes: prop[2,3]. This is
// not allowed by the current rules of standard C++
#include <map>
#ifndef USING_OLD_COMPILER
template <class Key, class T, class Compare = less<Key>,
         class Allocator = allocator<pair<const Key, T> > >
#else
template <class Key, class T>
#endif
class IndexedProperty
{
#ifndef USING_OLD_COMPILER
  std::map< Key, T, Compare, Allocator > data;
  typedef std::map< Key, T, Compare, Allocator >::iterator map_iterator;
#else
  std::map< Key, T > data;
  typedef std::map< Key, T >::iterator map_iterator;
#endif
public:
// function call syntax
  T operator()(Key const & key)
  {
    std::pair< map_iterator, bool > result;
    result = data.insert(std::make_pair(key, T()));
    return (*result.first).second;
  }
  T operator()(Key const & key, T const & t)
  {
    std::pair< map_iterator, bool > result;
    result = data.insert(std::make_pair(key, t));
    return (*result.first).second;
  }
// get/set syntax
  T get_Item(Key const & key)
  {
    std::pair< map_iterator, bool > result;
    result = data.insert(std::make_pair(key, T()));
    return (*result.first).second;
  }
  T set_Item(Key const & key, T const & t)
  {
    std::pair< map_iterator, bool > result;
    result = data.insert(std::make_pair(key, t));
    return (*result.first).second;
  }
// operator [] syntax
  T & operator[](Key const & key)
  {
    return (*((data.insert(make_pair(key, T()))).first)).second;
  }
};
