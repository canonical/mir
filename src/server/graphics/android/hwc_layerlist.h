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

enum HWCLayerType : unsigned int
{
    framebuffer = HWC_FRAMEBUFFER_TARGET, //a framebuffer layer
    gles        = HWC_FRAMEBUFFER,        //a layer to be composited using OpenGL
    overlay     = HWC_OVERLAY             //a layer to be composited as an overlay
};
 
struct HWCLayer : public hwc_layer_1
{
    virtual ~HWCLayer() = default;

    HWCLayer& operator=(HWCLayer const& layer)
    {
        *this = layer; 
        this->visibleRegionScreen = {1, &this->visible_rect};
        this->visible_rect = layer.visible_rect;
        this->must_use_gl = layer.must_use_gl;
        return *this;     
    }

    HWCLayer(HWCLayer const& layer)
    {
        *this = layer;
        this->visibleRegionScreen = {1, &this->visible_rect};
        this->visible_rect = layer.visible_rect;
        this->must_use_gl = layer.must_use_gl; 
    }

    HWCLayerType type() const;
protected:
    HWCLayer(HWCLayerType type, NativeBuffer const& buffer, bool must_use_gl);

    hwc_rect_t visible_rect;
    bool must_use_gl;
};

struct CompositionLayer : public HWCLayer
{
    CompositionLayer(NativeBuffer const&, bool must_use_gl);
};

struct FramebufferLayer : public HWCLayer
{
    FramebufferLayer(NativeBuffer const&);
};

class LayerList
{
public:
    LayerList(std::initializer_list<HWCLayer> const& layers);
    hwc_display_contents_1_t* native_list() const;

private:
    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
