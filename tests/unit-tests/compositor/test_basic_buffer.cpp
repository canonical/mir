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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

/* since we're testing functions that belong to a class that shouldn't have 
   a constructor, we test via a test class */

#include "mir/compositor/buffer_basic.h"
#include "mir/compositor/buffer_ipc_package.h"

#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry;

class TestClassBuffer : public mc::BufferBasic
{
public:
    TestClassBuffer()
    {
    }

    geom::Size size() const
    {
        return geom::Size{geom::Width{0}, geom::Height{0}};
    }

    geom::Stride stride() const
    {
        return geom::Stride{0};
    }

    geom::PixelFormat pixel_format() const
    {
        return geom::PixelFormat::abgr_8888;
    }

    void bind_to_texture()
    {
    }

    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const
    {
        return std::shared_ptr<mc::BufferIPCPackage>(); 
    }
};
