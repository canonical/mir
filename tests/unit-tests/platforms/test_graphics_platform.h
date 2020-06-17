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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef TEST_GRAPHICS_PLATFORM_H_
#define TEST_GRAPHICS_PLATFORM_H_

#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display.h"

#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/null_display_configuration_policy.h"

namespace mtd = mir::test::doubles;

TEST_F(GraphicsPlatform, buffer_allocator_creation)
{
    using namespace testing;

    EXPECT_NO_THROW (
        auto platform = create_platform();
        auto display = platform->create_display(
            std::make_shared<mtd::NullDisplayConfigurationPolicy>(),
            std::make_shared<mir::test::doubles::StubGLConfig>());
        auto allocator = platform->create_buffer_allocator(*display);

        EXPECT_TRUE(allocator.get());
    );
}

TEST_F(GraphicsPlatform, buffer_creation)
{
    auto platform = create_platform();
    auto display = platform->create_display(
        std::make_shared<mtd::NullDisplayConfigurationPolicy>(),
        std::make_shared<mir::test::doubles::StubGLConfig>());
    auto allocator = platform->create_buffer_allocator(*display);
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    ASSERT_NE(0u, supported_pixel_formats.size());

    geom::Size size{320, 240};
    MirPixelFormat const pf{supported_pixel_formats[0]};
    mg::BufferUsage usage{mg::BufferUsage::hardware};
    mg::BufferProperties buffer_properties{size, pf, usage};

    auto buffer = allocator->alloc_software_buffer(size, pf);

    ASSERT_TRUE(buffer.get() != NULL);

    EXPECT_EQ(buffer->size(), size);
    EXPECT_EQ(buffer->pixel_format(), pf);
}

TEST_F(GraphicsPlatform, connection_ipc_package)
{
    auto platform = create_platform();
    auto ipc_ops = platform->make_ipc_operations();
    auto pkg = ipc_ops->connection_ipc_package();

    ASSERT_TRUE(pkg.get() != NULL);
}

#endif // TEST_GRAPHICS_PLATFORM_H_
