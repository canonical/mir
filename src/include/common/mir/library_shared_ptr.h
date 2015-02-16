/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_LIBRARY_SHARED_PTR_H_
#define MIR_LIBRARY_SHARED_PTR_H_

#include <memory>
#include <type_traits>

namespace mir
{
class SharedLibrary;
/*!
 * \brief Extends life time of a shared library beyond that of a shared ptr
 */
template<typename T>
class LibrarySharedPtr
{
public:
    LibrarySharedPtr() = default;

    template<typename U,
        typename = typename std::enable_if<std::is_convertible<U,T>::value>>
    LibrarySharedPtr(std::shared_ptr<SharedLibrary> const& library,
                     std::shared_ptr<U> const& object)
        : lib{library}, object{object}
    {}

    template<typename U,
        typename = typename std::enable_if<std::is_convertible<U,T>::value>>
    LibrarySharedPtr(LibrarySharedPtr<U> const& other)
        : lib{other.lib}, object{other.object}
    {
    }

    template<typename U,
        typename = typename std::enable_if<std::is_convertible<U,T>::value>>
    LibrarySharedPtr& operator=(LibrarySharedPtr<U> const& other)
    {
        object = other.object;
        lib = other.lib;
        return *this;
    }

    void reset()
    {
        object.reset();
        lib.reset();
    }

    T& operator*() const
    {
        return *object;
    }
    T* operator->() const
    {
        return &*object;
    }

    template<typename U>
    bool operator==(LibrarySharedPtr<U> const& other) const
    {
        return object == other.object;
    }

    template<typename U>
    bool operator!=(LibrarySharedPtr<U> const& other) const
    {
        return object != other.object;
    }
    explicit operator bool()
    {
        return !!object;
    }

private:
    std::shared_ptr<SharedLibrary> lib;
    std::shared_ptr<T> object;
};

}

#endif
