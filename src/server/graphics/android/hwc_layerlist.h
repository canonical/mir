/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_
#define MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_

#include "mir/geometry/rectangle.h"
#include <hardware/hwcomposer.h>
#include <memory>
#include <vector>

namespace mir
{
namespace compositor
{
class Buffer;
class NativeBufferHandle;
}
namespace graphics
{
namespace android
{

class HWCLayer;
class LayerListManager
{
    //interface
};

typedef struct std::vector<std::shared_ptr<HWCLayer>> LayerList; 
class HWCLayerList : public LayerListManager
{
public:
    HWCLayerList();
    const LayerList& native_list() const;

    void set_fb_target(std::shared_ptr<compositor::Buffer> const&);

private:
    LayerList layer_list;
};

}
}
}


namespace mir
{
namespace graphics
{
namespace android
{
//construction is a bit funny because hwc_layer_1 has unions
struct HWCRect : public hwc_rect_t
{
    HWCRect(); 
    HWCRect(geometry::Rectangle& rect);
};

struct HWCLayer : public hwc_layer_1
{
    HWCLayer(
        std::shared_ptr<compositor::NativeBufferHandle> const& native_buf,
        HWCRect& source_crop_rect,
        HWCRect& display_frame_rect,
        HWCRect& visible_rect);

private:
    HWCRect visible_screen_rect;
    //remember to disallow copy/assign
};
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
