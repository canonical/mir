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
std::shared_ptr<mir::SharedLibrary> get_shared_library(void *address)
{
    if (!address)
        return nullptr;

    Dl_info library_info{nullptr, nullptr, nullptr, nullptr};
    Dl_info executable_info{nullptr, nullptr, nullptr, nullptr};
    
    dladdr(dlsym(nullptr, "main"), &executable_info);
    dladdr(address, &library_info);
    
    if (library_info.dli_fbase == executable_info.dli_fbase)
        return nullptr;
            
    return std::make_shared<mir::SharedLibrary>(library_info.dli_fname);
}
}
// this class serves two purposes, supporting default construction of
// UniqueModulePtr and ensuring that platform modules do not inline
// any of the shared_ptr<mir::SharedLibrary> which would be unmapped from
// the process during destruction.
mir::detail::RefCountedLibrary::RefCountedLibrary(void* address)
    : internal_state(get_shared_library(address))
{
}

mir::detail::RefCountedLibrary::RefCountedLibrary(RefCountedLibrary const&) = default;
mir::detail::RefCountedLibrary::~RefCountedLibrary() = default;
mir::detail::RefCountedLibrary& mir::detail::RefCountedLibrary::operator=(RefCountedLibrary const&) = default;

