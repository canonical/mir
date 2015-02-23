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

#include "mir/module_deleter.h"

#include "mir/shared_library.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <atomic>

namespace
{
char const* get_library_name(void *address)
{
    Dl_info info{nullptr, nullptr, nullptr, nullptr};
    dladdr(address, &info);
    return info.dli_fname;
}
}

// this class serves two purposes, supporting default construction of
// UniqueModulePtr and ensuring that platform modules do not inline
// any of the shared_ptr<mir::SharedLibrary> which would be unmapped from
// the process during destruction.
mir::detail::RefCountedLibrary::RefCountedLibrary(void* address)
    : internal_state(
        address ?
        std::make_shared<mir::SharedLibrary>(
            get_library_name(address)
            ) :
        nullptr)
{
}

mir::detail::RefCountedLibrary::RefCountedLibrary(RefCountedLibrary const&) = default;
mir::detail::RefCountedLibrary::~RefCountedLibrary() = default;
mir::detail::RefCountedLibrary& mir::detail::RefCountedLibrary::operator=(RefCountedLibrary const&) = default;

