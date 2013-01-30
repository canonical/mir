/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_VECTOR_H_
#define MIR_ANDROID_UBUNTU_VECTOR_H_

#include <vector>

namespace android
{
template <typename ValueType>
class SortedVector;

template <typename ValueType>
class Vector
{
    typedef std::vector<ValueType> Impl;
public:
    typedef typename Impl::value_type value_type;

    /*!
     * Constructors and destructors
     */
                            Vector() {}
                            Vector(const Vector<ValueType>& rhs) : impl(rhs.impl) {}
//    explicit                Vector(const SortedVector<ValueType>& rhs);
    virtual                 ~Vector() {}

    /*! copy operator */
//            const Vector<ValueType>&     operator = (const Vector<ValueType>& rhs) const;
            Vector<ValueType>&           operator = (const Vector<ValueType>& rhs) { impl = rhs.impl; }

//            const Vector<ValueType>&     operator = (const SortedVector<ValueType>& rhs) const;
//            Vector<ValueType>&           operator = (const SortedVector<ValueType>& rhs);

            /*
     * empty the vector
     */

    inline  void            clear()             { impl.clear(); }

    /*!
     * vector stats
     */

    //! returns number of items in the vector
    inline  size_t          size() const                { return impl.size(); }
    //! returns wether or not the vector is empty
    inline  bool            isEmpty() const             { return impl.empty(); }
    //! returns how many items can be stored without reallocating the backing store
    inline  size_t          capacity() const            { return impl.capacity(); }
    //! setst the capacity. capacity can never be reduced less than size()
    inline  ssize_t         setCapacity(size_t size)    { impl.reserve(size); return size; }

    /*!
     * C-style array access
     */

    //! read-only C-style access
    inline  const ValueType*     array() const { return impl.data(); }
    //! read-write C-style access
            ValueType*           editArray() { return impl.data(); }

    /*!
     * accessors
     */

    //! read-only access to an item at a given index
    inline  const ValueType&     operator [] (size_t index) const { return impl[index]; }
    //! alternate name for operator []
    inline  const ValueType&     itemAt(size_t index) const { return impl.at(index); }
    //! stack-usage of the vector. returns the top of the stack (last element)
            const ValueType&     top() const { return impl.back(); }
    //! same as operator [], but allows to access the vector backward (from the end) with a negative index
            const ValueType&     mirrorItemAt(ssize_t index) const
            { return impl.at((index >= 0) ? index : -index); }

    /*!
     * modifing the array
     */

    //! copy-on write support, grants write access to an item
            ValueType&           editItemAt(size_t index) { return impl[index]; }
    //! grants right acces to the top of the stack (last element)
            ValueType&           editTop() { return impl.back(); }

            /*!
             * append/insert another vector
             */

//    //! insert another vector at a given index
//            ssize_t         insertVectorAt(const Vector<ValueType>& vector, size_t index);

    //! append another vector at the end of this one
            ssize_t         appendVector(const Vector<ValueType>& vector)
            {
                auto result = impl.size();
                impl.insert(impl.end(), vector.begin(), vector.end());
                return result;
            }


//    //! insert an array at a given index
//            ssize_t         insertArrayAt(const ValueType* array, size_t index, size_t length);

    //! append an array at the end of this vector
            ssize_t         appendArray(const ValueType* array, size_t length)
            {
                auto result = impl.size();
                impl.insert(impl.end(), array, array + length);
                return result;
            }

            /*!
             * add/insert/replace items
             */

    //! insert one or several items initialized with their default constructor
    inline  ssize_t         insertAt(size_t index, size_t numItems = 1)
    { return insertAt(ValueType(), index, numItems); }
    //! insert one or several items initialized from a prototype item
            ssize_t         insertAt(const ValueType& prototype_item, size_t index, size_t numItems = 1)
    {   impl.insert(impl.begin()+index, numItems, prototype_item); return index; }
    //! pop the top of the stack (removes the last element). No-op if the stack's empty
    inline  void            pop() { if (!impl.empty()) impl.erase(--impl.end()); }
    //! pushes an item initialized with its default constructor
    inline  void            push() { impl.push_back(ValueType()); }
    //! pushes an item on the top of the stack
            void            push(const ValueType& item) { impl.push_back(item); }
    //! same as push() but returns the index the item was added at (or an error)
    inline  ssize_t         add()
    {
        auto result = impl.size();
        push();
        return result;
    }

    //! same as push() but returns the index the item was added at (or an error)
            ssize_t         add(const ValueType& item)
            {
                auto result = impl.size();
                push(item);
                return result;
            }
//    //! replace an item with a new one initialized with its default constructor
//    inline  ssize_t         replaceAt(size_t index);
//    //! replace an item with a new one
//            ssize_t         replaceAt(const ValueType& item, size_t index);

    /*!
     * remove items
     */

    //! remove several items
    inline  ssize_t         removeItemsAt(size_t index, size_t count = 1)
    { auto i = impl.begin() + index; impl.erase(i, i+count); return index; }
    //! remove one item
    inline  ssize_t         removeAt(size_t index)
    { auto i = impl.begin() + index; impl.erase(i); return index; }

//    /*!
//     * sort (stable) the array
//     */
//
//     typedef int (*compar_t)(const ValueType* lhs, const ValueType* rhs);
//     typedef int (*compar_r_t)(const ValueType* lhs, const ValueType* rhs, void* state);
//
//     inline status_t        sort(compar_t cmp);
//     inline status_t        sort(compar_r_t cmp, void* state);
//
//     // for debugging only
//     inline size_t getItemSize() const { return itemSize(); }


     /*
      * these inlines add some level of compatibility with STL. eventually
      * we should probably turn things around.
      */
     typedef typename Impl::iterator iterator;
     typedef typename Impl::const_iterator const_iterator;

     inline iterator begin() { return impl.begin(); }
     inline iterator end()   { return impl.end(); }
     inline const_iterator begin() const { return impl.begin(); }
     inline const_iterator end() const   { return impl.end(); }
     inline void reserve(size_t n) { impl.reserve(n); }
     inline bool empty() const{ return impl.empty(); }
     inline void push_back(const ValueType& item)  { impl.push_back(); }
     inline void push_front(const ValueType& item) { impl.push_front(); }
     inline iterator erase(iterator pos) { return impl.erase(pos); }

private:
    Impl impl;
};
}

#endif /* MIR_ANDROID_UBUNTU_VECTOR_H_ */
