/*
 * Copyright © 2012-2014 Canonical Ltd.
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
#ifndef MIR_TEST_DRAW_GRAPHICS_REGION_FACTORY
#define MIR_TEST_DRAW_GRAPHICS_REGION_FACTORY

#include "mir/graphics/native_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include <hardware/gralloc.h>
#include <memory>

namespace mir
{
namespace test
{
class GraphicsRegionFactory
{
public:
    GraphicsRegionFactory();
    ~GraphicsRegionFactory();
    std::shared_ptr<MirGraphicsRegion> graphic_region_from_handle(
        graphics::NativeBuffer& native_buffer);

protected:
    GraphicsRegionFactory(GraphicsRegionFactory const&) = delete;
    GraphicsRegionFactory& operator=(GraphicsRegionFactory const&) = delete;

private:
    gralloc_module_t* module;
    alloc_device_t* alloc_dev;
};
}
}
#endif /* MIR_TEST_DRAW_GRAPHICS_REGION_FACTORY */
