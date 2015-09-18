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

#include <memory>

namespace mir_test_framework
{
class UsingStubClientPlatform
{
public:
    UsingStubClientPlatform();
    ~UsingStubClientPlatform();

    UsingStubClientPlatform(UsingStubClientPlatform const&) = delete;
    UsingStubClientPlatform operator=(UsingStubClientPlatform const&) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
}

#endif /* MIR_TEST_FRAMEWORK_USING_STUB_CLIENT_PLATFORM_H_ */
