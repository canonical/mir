/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test_framework/stub_client_platform_options.h"
#include "mir/shared_library.h"
#include "mir_test_framework/executable_path.h"

#include <type_traits>

namespace mtf = mir_test_framework;

namespace
{
mir::SharedLibrary& persistent_client_library()
{
    // Deliberately leak the client library to avoid any destructor unpleasantness.
    static mir::SharedLibrary* const client_library
        = new mir::SharedLibrary{mtf::client_platform("dummy.so")};

    return *client_library;
}
}

void mtf::add_client_platform_error(FailurePoint where, std::exception_ptr what)
{
    auto error_adder = persistent_client_library()
        .load_function<void(*)(FailurePoint, std::exception_ptr)>("add_client_platform_error");

    error_adder(where, what);
}
