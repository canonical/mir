/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_PLATFORM_AUTHENTICATION_H_
#define MIR_TEST_DOUBLES_MOCK_PLATFORM_AUTHENTICATION_H_

#include "mir/graphics/platform_authentication.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockPlatformAuthentication : graphics::PlatformAuthentication
{
    MOCK_METHOD0(platform_fd_items, std::vector<int>());
    MOCK_METHOD0(auth_extension, optional_value<std::shared_ptr<graphics::MesaAuthExtension>>());
    MOCK_METHOD0(set_gbm_extension, optional_value<std::shared_ptr<graphics::SetGbmExtension>>());
    MOCK_METHOD0(drm_fd, optional_value<mir::Fd>());
    MOCK_METHOD2(request_interface, void*(char const*, int));
};

}
}
}

#endif
