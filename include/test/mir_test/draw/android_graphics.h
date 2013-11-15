/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DRAW_ANDROID_GRAPHICS
#define MIR_TEST_DRAW_ANDROID_GRAPHICS

#include "mir_toolkit/mir_client_library.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/size.h"

#include <hardware/gralloc.h>
#include <memory>

namespace mir
{
namespace compositor
{
    class BufferIPCPackage;
}
namespace test
{
namespace draw
{

class TestGrallocMapper
{
public:
    TestGrallocMapper();
    TestGrallocMapper(const hw_module_t *hw_module, alloc_device_t* alloc_dev);
    ~TestGrallocMapper();
    std::shared_ptr<MirGraphicsRegion> graphic_region_from_handle(ANativeWindowBuffer* package);

private:
    TestGrallocMapper(TestGrallocMapper const&) = delete;
    TestGrallocMapper& operator=(TestGrallocMapper const&) = delete;

    const bool gralloc_ownership;
    gralloc_module_t* module;
    alloc_device_t* alloc_dev;
};

}
}
}
#endif /* MIR_TEST_DRAW_ANDROID_GRAPHICS */
