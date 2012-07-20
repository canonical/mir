/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

TEST(GraphicsPlatform, buffer_allocator)
{
    std::unique_ptr<mc::GraphicBufferAllocator> allocator = mg::create_buffer_allocator();

    ASSERT_NE(allocator.get(), nullptr);
}
