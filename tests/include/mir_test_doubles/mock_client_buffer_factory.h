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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_FACTORY_H_

#include "mir/client_buffer_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockClientBufferFactory : client::ClientBufferFactory
{
    MOCK_METHOD3(create_buffer,
                 std::shared_ptr<client::ClientBuffer>(
                    std::shared_ptr<MirBufferPackage> const& /*package*/,
                    mir::geometry::Size /*size*/, MirPixelFormat /*pf*/));
};
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_FACTORY_H_
