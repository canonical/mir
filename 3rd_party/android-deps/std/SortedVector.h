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


#ifndef MIR_ANDROID_UBUNTU_SORTEDVECTOR_H_
#define MIR_ANDROID_UBUNTU_SORTEDVECTOR_H_

#include <std/Errors.h>

#include <algorithm>
#include <vector>

namespace mir_input
{
template <class ValueType>
class SortedVector : std::vector<ValueType>
{
    typedef std::vector<ValueType> Impl;
    using Impl::begin;
    using Impl::end;
    using Impl::insert;
    using Impl::erase;
    using Impl::operator[];
public:
    using Impl::empty;
    using Impl::clear;
    using Impl::size;

    /*
    * the following inlines add some level of compatibility with android utils.
    * Stuff that is commented out isn't used in the input stack
    */

//            typedef ValueType    value_type;
//
//    /*!
//     * Constructors and destructors
//     */
//
//                            SortedVector();
//                            SortedVector(const SortedVector<ValueType>& rhs);
//    virtual                 ~SortedVector();
//
//    /*! copy operator */
//    const SortedVector<ValueType>&   operator = (const SortedVector<ValueType>& rhs) const;
//    SortedVector<ValueType>&         operator = (const SortedVector<ValueType>& rhs);

    /*!
     * vector stats
     */

    //! returns wether or not the vector is empty
    bool isEmpty() const { return empty(); }
//    //! returns how many items can be stored without reallocating the backing store
//    inline  size_t          capacity() const            { return VectorImpl::capacity(); }
//    //! setst the capacity. capacity can never be reduced less than size()
//    inline  ssize_t         setCapacity(size_t size)    { return VectorImpl::setCapacity(size); }
//
//    /*!
//     * C-style array access
//     */
//
//    //! read-only C-style access
//    inline  const ValueType*     array() const;
//
//    //! read-write C-style access. BE VERY CAREFUL when modifying the array
//    //! you ust keep it sorted! You usually don't use this function.
//            ValueType*           editArray();

    //! finds the index of an item
    ssize_t indexOf(const ValueType& item) const
    {
        auto p = lower_bound(begin(), end(), item);

        if (p != end() && *p == item) return distance(begin(), p);
        else return NAME_NOT_FOUND;
    }

//            //! finds where this item should be inserted
//            size_t          orderOf(const ValueType& item) const;


    /*!
     * accessors
     */

//    //! read-only access to an item at a given index
//    inline  const ValueType&     operator [] (size_t index) const;
    //! alternate name for operator []
    const ValueType& itemAt(size_t index) const { return operator[](index); }
//     stack-usage of the vector. returns the top of the stack (last element)
//            const ValueType&     top() const;
//    //! same as operator [], but allows to access the vector backward (from the end) with a negative index
//            const ValueType&     mirrorItemAt(ssize_t index) const;
//
    /*!
     * modifing the array
     */

    //! add an item in the right place (and replace the one that is there)
    ssize_t add(const ValueType& item)
    {
        auto pos = lower_bound(begin(), end(), item);
        if (pos == end() || !(*pos == item)) pos = insert(pos, item);
        else *pos = item;

        return distance(begin(), pos);
    }

    //! editItemAt() MUST NOT change the order of this item
    ValueType& editItemAt(size_t index)  { return operator[](index); }

//            //! merges a vector into this one
//            ssize_t         merge(const Vector<ValueType>& vector);
//            ssize_t         merge(const SortedVector<ValueType>& vector);

    //! removes an item
    ssize_t remove(const ValueType& item)
    {
        auto remove_pos = lower_bound(begin(), end(), item);
        if (remove_pos != end() && *remove_pos == item)
        {
            auto removed_at = erase(remove_pos);
            return distance(begin(), removed_at);
        }
        return NAME_NOT_FOUND;
    }


    //! remove several items
    ssize_t removeItemsAt(size_t index, size_t count = 1)
    { if (index+count > size()) return BAD_VALUE; auto i = begin() + index; erase(i, i+count); return index; }
//    //! remove one item
//    inline  ssize_t         removeAt(size_t index)  { return removeItemsAt(index); }
};
}

namespace android
{
using ::mir_input::SortedVector;
} // namespace android

#endif /* MIR_ANDROID_UBUNTU_SORTEDVECTOR_H_ */
