/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_NESTED_CONTEXT_H_
#define MIR_TEST_DOUBLES_MOCK_NESTED_CONTEXT_H_

#include "mir/graphics/nested_context.h"
#include "mir/graphics/platform_operation_message.h"

namespace mir
{
namespace graphics
{
inline bool operator==(PlatformOperationMessage const& msg1,
                       PlatformOperationMessage const& msg2)
{
    return msg1.data == msg2.data && msg1.fds == msg2.fds;
}

inline bool operator!=(PlatformOperationMessage const& msg1,
                       PlatformOperationMessage const& msg2)
{
    return !(msg1 == msg2);
}
}

namespace test
{
namespace doubles
{

struct MockNestedContext : graphics::NestedContext
{
    MOCK_METHOD0(platform_fd_items, std::vector<int>());
    MOCK_METHOD1(drm_auth_magic, void(int magic));
    MOCK_METHOD2(platform_operation, graphics::PlatformOperationMessage(
        unsigned int, graphics::PlatformOperationMessage const&));
};

}
}
}

#endif
