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

#include "mir/shared_library.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <memory>

namespace mir
{
namespace detail
{
namespace
{
inline char const* this_library()
{
    Dl_info info{nullptr, nullptr, nullptr, nullptr};
    dladdr(reinterpret_cast<void*>(&this_library), &info);
    return info.dli_fname;
}
}
}

const struct ModuleDeleterTag {} from_this_library{};

template<typename T>
struct ModuleDeleter : std::default_delete<T>
{
    ModuleDeleter(ModuleDeleterTag /*from_this_library*/)
        : library_closer{std::make_shared<SharedLibrary>(detail::this_library())}
    {}
    ModuleDeleter()
        : library_closer()
    {
    }
    std::shared_ptr<SharedLibrary> library_closer;
};

/*!
 * \brief Use UniqueModulePtr to ensure that your loadable libray outlives
 * instances created within it.
 *
 * Pass mir::from_this_library to the constructor, to increase the lifetime of your library:
 * \code
 *  mir::UniqueModulePtr<ExampleInterface> library_entry_point()
 *  {
 *      return mir::UniqueModulePtr<SomeInterface>(new Implementation, mir::from_this_library);
 *  }
 * \endcode
 *
 * The default constructor will not try to infer the dynamic library.
 */
template<typename T>
using UniqueModulePtr = std::unique_ptr<T,ModuleDeleter<T>>;
}

#endif

