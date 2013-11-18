/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_
#define MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_

#include "mir/graphics/android/fence.h"
#include "mir/geometry/rectangle.h"
#include <hardware/hwcomposer.h>
#include <memory>
#include <vector>
#include <initializer_list>

namespace mir
{
namespace graphics
{

class NativeBuffer;
class Buffer;

namespace android
{

struct HWCLayer : public hwc_layer_1
{
    virtual ~HWCLayer() = default;

    HWCLayer& operator=(HWCLayer const& layer);
    HWCLayer(HWCLayer const& layer);

protected:
    HWCLayer(int type, buffer_handle_t handle, int width, int height, int layer_flags);

    hwc_rect_t visible_rect;
};

struct CompositionLayer : public HWCLayer
{
    CompositionLayer(int layer_flags);
    CompositionLayer(NativeBuffer const&, int layer_flags);
};

struct FramebufferLayer : public HWCLayer
{
    FramebufferLayer();
    FramebufferLayer(NativeBuffer const&);
};

class LayerList
{
public:
    LayerList(std::initializer_list<HWCLayer> const& layers);

    hwc_display_contents_1_t* native_list() const;

    void set_fb_target(std::shared_ptr<NativeBuffer> const&);
    NativeFence framebuffer_fence();

private:
    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
