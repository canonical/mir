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

#ifndef MIR_MODULE_DELETER_H_
#define MIR_MODULE_DELETER_H_

#include <memory>

namespace mir
{
class SharedLibrary;

namespace detail
{
class RefCountedLibrary
{
public:
    RefCountedLibrary(void* address);
    RefCountedLibrary(RefCountedLibrary const&);
    ~RefCountedLibrary();
    RefCountedLibrary& operator=(RefCountedLibrary const&);
private:
    std::shared_ptr<mir::SharedLibrary> internal_state;
};
}

template<typename T>
struct ModuleDeleter : std::default_delete<T>
{
    ModuleDeleter() : library(nullptr) {}
    template<typename U>
    ModuleDeleter(ModuleDeleter<U> const& other)
        : std::default_delete<T>{other},
        library{other.get_library()}
    {
    }

    detail::RefCountedLibrary get_library() const
    {
        return library;
    }

protected:
    ModuleDeleter(void *address_in_module)
        : library{address_in_module}
    {
    }
private:
    detail::RefCountedLibrary library;
};

/*!
 * \brief Use UniqueModulePtr to ensure that your loadable libray outlives
 * instances created within it.
 *
 * Use mir::make_module_ptr(...) or pass a function from your library to the
 * constructor, to increase the lifetime of your library:
 * \code
 *  mir::UniqueModulePtr<ExampleInterface> library_entry_point()
 *  {
 *      return mir::UniqueModulePtr<SomeInterface>(new Implementation, &library_entry_point);
 *  }
 * \endcode
 *
 * The default constructor will not try to infer the dynamic library.
 */
template<typename T>
using UniqueModulePtr = std::unique_ptr<T,ModuleDeleter<T>>;

namespace
{
/*!
 * \brief make_unique like creation function for UniqueModulePtr
 */
template<typename Type, typename... Args>
inline auto make_module_ptr(Args&&... args)
-> UniqueModulePtr<Type>
{
    struct Deleter : ModuleDeleter<Type>
    {
        Deleter(void* address)
            : ModuleDeleter<Type>(address) {}
    } deleter(reinterpret_cast<void*>(&make_module_ptr<Type, Args...>));

    return UniqueModulePtr<Type>(new Type(std::forward<Args>(args)...), std::move(deleter));
}
}

}
#endif
