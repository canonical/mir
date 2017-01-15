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

#ifndef MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_OPTIONS_H_
#define MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_OPTIONS_H_

#include <exception>

namespace mir_test_framework
{
/*
 * TODO: This should be an enum class, but gcc < 6 doesn't implement the
 * C++14 DR requiring that std::unordered_map<enum, FOO> works, and using
 * a raw enum means we can pass std::hash<int> rather than adding an extra
 * hash implementation.
 */
enum FailurePoint
{
    create_client_platform,
    create_egl_native_window,
    create_buffer_factory
};

/**
 * Add an exception to the client platform created by the \b next call to create_client_platform
 *
 * \param [in]      where   The platform call that will throw an exception
 * \param [in,out]  what    The exception to throw
 */
void add_client_platform_error(FailurePoint where, std::exception_ptr what);
}

#endif //MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_OPTIONS_H_
