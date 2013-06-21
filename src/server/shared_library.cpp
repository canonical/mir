/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/shared_library.h"

#include <dlfcn.h>

#include <iostream>
#include <stdexcept>

mir::SharedLibrary::SharedLibrary(char const* library_name) :
    so(dlopen(library_name, RTLD_NOW))
{
    if (!so)
    {
        // TODO proper error reporting
        std::cerr << "Cannot open library: " << dlerror() << std::endl;
        throw std::runtime_error("Cannot open library");
    }
}

mir::SharedLibrary::SharedLibrary(std::string const& library_name) :
    SharedLibrary(library_name.c_str()) {}

mir::SharedLibrary::~SharedLibrary()
{
    dlclose(so);
}

void* mir::SharedLibrary::load_symbol(char const* function_name) const
{
    if (void* result = dlsym(so, function_name))
    {
        return result;
    }
    else
    {
        // TODO proper error reporting
        std::cerr << "Cannot load symbol: " << dlerror() << std::endl;
        throw std::runtime_error("Cannot load symbol");
    }
}
