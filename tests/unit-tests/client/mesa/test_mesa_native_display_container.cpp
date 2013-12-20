/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/client/mesa/mesa_native_display_container.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mesa/native_display.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

namespace mclg = mir::client::mesa;

namespace
{

struct MesaNativeDisplayContainerSetup : public testing::Test
{
    MesaNativeDisplayContainerSetup()
        : container(std::make_shared<mclg::MesaNativeDisplayContainer>()),
          connection(nullptr)
    {
    }

    std::shared_ptr<mclg::MesaNativeDisplayContainer> const container;
    MirConnection* connection;
};

}

TEST_F(MesaNativeDisplayContainerSetup, valid_displays_come_from_factory)
{
    using namespace ::testing;

    auto display = container->create(connection);
    EXPECT_TRUE(container->validate(display));

    MirEGLNativeDisplayType invalid_native_display;
    EXPECT_FALSE(container->validate(&invalid_native_display));
}

TEST_F(MesaNativeDisplayContainerSetup, releasing_displays_invalidates_address)
{
    using namespace ::testing;

    auto display = container->create(connection);
    EXPECT_TRUE(container->validate(display));
    container->release(display);
    EXPECT_FALSE(container->validate(display));
}
