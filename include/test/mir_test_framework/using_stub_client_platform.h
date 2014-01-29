/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_USING_STUB_CLIENT_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_USING_STUB_CLIENT_PLATFORM_H_

#include "src/client/api_impl_types.h"

namespace mir_test_framework
{

class UsingStubClientPlatform
{
public:
    UsingStubClientPlatform();
    ~UsingStubClientPlatform();

private:
    UsingStubClientPlatform(UsingStubClientPlatform const&) = delete;
    UsingStubClientPlatform operator=(UsingStubClientPlatform const&) = delete;

    mir_connect_impl_func const prev_mir_connect_impl;
    mir_connection_release_impl_func const prev_mir_connection_release_impl;
};

}

#endif /* MIR_TEST_FRAMEWORK_USING_STUB_CLIENT_PLATFORM_H_ */
