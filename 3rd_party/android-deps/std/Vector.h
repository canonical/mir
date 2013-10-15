/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_VECTOR_H_
#define MIR_ANDROID_UBUNTU_VECTOR_H_

#include <vector>

namespace mir_input
{
template <typename ValueType>
class Vector : std::vector<ValueType> // NB private inheritance of implementation
{
    typedef std::vector<ValueType> Impl;
public:

    using typename Impl::value_type;
    using typename Impl::iterator;
    using typename Impl::const_iterator;

    using Impl::clear;
    using Impl::size;
    using Impl::capacity;
    using Impl::begin;
    using Impl::end;
    using Impl::reserve;
    using Impl::empty;
    using Impl::push_back;
    using Impl::erase;
    using Impl::insert;
    using Impl::at;
    using Impl::operator[];

    /*
    * the following inlines add some level of compatibility with android utils.
    * Stuff that is commented out isn't used in the input stack
    */

    /*!
    * Constructors and destructors
    */
    Vector() = default;
    Vector(const Vector<ValueType>& /* rhs */) = default;
//  explicit Vector(const SortedVector<ValueType>& rhs);
    virtual ~Vector() {}

    /*! copy operator */
//  const Vector<ValueType>& operator = (const Vector<ValueType>& rhs) const;
    Vector<ValueType>& operator=(const Vector<ValueType>& /* rhs */) = default;

//  const Vector<ValueType>& operator = (const SortedVector<ValueType>& rhs) const;
//  Vector<ValueType>& operator = (const SortedVector<ValueType>& rhs);


    /*!
    * vector stats
    */

    //! returns wether or not the vector is empty
    inline bool isEmpty() const { return empty(); }
    //! setst the capacity. capacity can never be reduced less than size()
    inline ssize_t setCapacity(size_t size) { reserve(size); return size; }

    /*!
    * C-style array access
    */

    //! read-only C-style access
    inline const ValueType* array() const { return Impl::data(); }
    //! read-write C-style access
    ValueType* editArray() { return Impl::data(); }

    /*!
    * accessors
    */

    //! alternate name for operator []
    inline const ValueType& itemAt(size_t index) const { return at(index); }
    //! stack-usage of the vector. returns the top of the stack (last element)
    const ValueType& top() const { return Impl::back(); }
//  //! same as operator [], but allows to access the vector backward (from the end) with a negative index
//  const ValueType& mirrorItemAt(ssize_t index) const { return at((index >= 0) ? index : size()+index); }

    /*!
    * modifing the array
    */

    //! copy-on write support, grants write access to an item
    ValueType& editItemAt(size_t index) { return Impl::operator[](index); }
    //! grants right acces to the top of the stack (last element)
    ValueType& editTop() { return Impl::back(); }

    /*!
    * append/insert another vector
    */

//  //! insert another vector at a given index
//  ssize_t insertVectorAt(const Vector<ValueType>& vector, size_t index);

    //! append another vector at the end of this one
    ssize_t appendVector(const Vector<ValueType>& vector)
    {
        auto result = size();
        insert(end(), vector.begin(), vector.end());
        return result;
    }


//  //! insert an array at a given index
//  ssize_t insertArrayAt(const ValueType* array, size_t index, size_t length);

    //! append an array at the end of this vector
    ssize_t appendArray(const ValueType* array, size_t length)
    {
        auto result = size();
        insert(end(), array, array + length);
        return result;
    }

    /*!
    * add/insert/replace items
    */

    //! insert one or several items initialized with their default constructor
    inline ssize_t insertAt(size_t index, size_t numItems = 1) { return insertAt(ValueType(), index, numItems); }
    //! insert one or several items initialized from a prototype item
    ssize_t insertAt(const ValueType& prototype_item, size_t index, size_t numItems = 1)
    { insert(begin()+index, numItems, prototype_item); return index; }
    //! pop the top of the stack (removes the last element). No-op if the stack's empty
    inline void pop() { if (!empty()) erase(--end()); }
    //! pushes an item initialized with its default constructor
    inline void push() { push_back(ValueType()); }
    //! pushes an item on the top of the stack
    void push(const ValueType& item) { push_back(item); }
    //! same as push() but returns the index the item was added at (or an error)
    inline ssize_t add() { auto result = size(); push(); return result; }

    //! same as push() but returns the index the item was added at (or an error)
    ssize_t add(const ValueType& item) { auto result = size(); push(item); return result; }
//  //! replace an item with a new one initialized with its default constructor
//  inline ssize_t replaceAt(size_t index);
//  //! replace an item with a new one
//  ssize_t replaceAt(const ValueType& item, size_t index);

    /*!
    * remove items
    */

//! remove several items
    inline ssize_t removeItemsAt(size_t index, size_t count = 1)
    { auto i = begin() + index; erase(i, i+count); return index; }
//! remove one item
    inline ssize_t removeAt(size_t index)
    { auto i = begin() + index; erase(i); return index; }

//  /*!
//  * sort (stable) the array
//  */
//
//  typedef int (*compar_t)(const ValueType* lhs, const ValueType* rhs);
//  typedef int (*compar_r_t)(const ValueType* lhs, const ValueType* rhs, void* state);
//
//  inline status_t sort(compar_t cmp);
//  inline status_t sort(compar_r_t cmp, void* state);
//
//  // for debugging only
//  inline size_t getItemSize() const { return itemSize(); }
};
}
namespace android
{
using ::mir_input::Vector;
}

#endif /* MIR_ANDROID_UBUNTU_VECTOR_H_ */
