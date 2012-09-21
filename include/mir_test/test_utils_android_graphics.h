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
#ifndef MIR_TEST_TEST_UTILS_ANDROID_GRAPHICS
#define MIR_TEST_TEST_UTILS_ANDROID_GRAPHICS

#include "mir/compositor/buffer_bundle.h"
#include "mir/geometry/dimensions.h"

#include <hardware/gralloc.h>
#include <memory>

namespace mir
{
namespace test
{

class grallocRenderSW
{
public:
    grallocRenderSW(); 
    ~grallocRenderSW(); 
    void render_pattern(std::shared_ptr<compositor::GraphicBufferClientResource>, 
                        geometry::Width w, geometry::Height h, int val );
 
private:
    gralloc_module_t* module;
    alloc_device_t* alloc_dev;
};

}
}

#endif /* MIR_TEST_TEST_UTILS_ANDROID_GRAPHICS */
