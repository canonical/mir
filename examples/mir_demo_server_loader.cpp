/*
 * Copyright © 2015 Canonical Ltd.
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

#include <dlfcn.h>
#include <stdexcept>
#include <iostream>

namespace
{
const char* const library = "libmir_demo_server_loadable.so";
const char* const entry = "main";
}

int main(int argc, char const* argv[])
try
{
    auto const so = dlopen(library, RTLD_NOW|RTLD_LOCAL);
    if (!so) throw std::runtime_error(dlerror());

    int (*loaded_main)(int, char const*[]){nullptr};

    (void*&)loaded_main = dlsym(so, entry);
    if (!loaded_main) throw std::runtime_error(dlerror());

    return loaded_main(argc, argv);

}
catch(std::exception const& x)
{
    std::cerr << "Error:" << x.what() << std::endl;
}
