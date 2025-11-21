/*
 * Copyright © Canonical Ltd.
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
 */

#include "test_display_platform.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/throw_exception.hpp>

#include <mir/graphics/platform.h>

namespace mg = mir::graphics;

DisplayPlatformTest::DisplayPlatformTest()
{
    GetParam()->setup();
}

DisplayPlatformTest::~DisplayPlatformTest()
{
    GetParam()->teardown();
}

TEST_P(DisplayPlatformTest, has_display_platform_entrypoints)
{
    auto const module = GetParam()->platform_module();

    try
    {
        module->load_function<mg::DescribeModule>(
            "describe_graphics_module",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
            << "Failed to find describe_graphics_module (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
            << err.what();
    }

    try
    {
        module->load_function<mg::CreateRenderPlatform>(
            "create_display_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
            << "Failed to find create_display_platform (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
            << err.what();
    }

    try
    {
        module->load_function<mg::PlatformProbe>(
            "probe_display_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
                << "Failed to find probe_display_platform (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
                << err.what();
    }
}
