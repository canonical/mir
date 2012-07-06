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
 * Authored by:
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/android/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mca = mir::compositor::android;
namespace geom = mir::geometry;

TEST(android_graphics_buffer_allocator, returns_non_empty_buffer)
{
    geom::Width w{1024};
    geom::Height h{768};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};
    
    mir::compositor::android::GraphicBufferAllocator allocator;
    auto buffer = allocator.alloc_buffer(w, h, pf);

    EXPECT_TRUE(buffer.get());
}
