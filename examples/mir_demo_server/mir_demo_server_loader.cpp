/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include <thread>

namespace
{
const char* const library = "libmir_demo_server_loadable.so";
const char* const entry = "main";

// Work around gold (or gcc/libstdc++-4.9 bug, it's not yet clear) bug
// https://sourceware.org/bugzilla/show_bug.cgi?id=16417 by ensuring the
// executable links with libpthread. A similar problem, where the pthread
// link dependency mysteriously disappears, also exists when using LTO
// with ld.bfd.
struct GoldBug16417Workaround
{
    GoldBug16417Workaround()
    {
        std::thread{[]{}}.join();
    }
} gold_bug_16417_workaround;
}

int main(int argc, char const* argv[])
try
{
    auto const so = dlopen(library, RTLD_LAZY|RTLD_LOCAL);
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
