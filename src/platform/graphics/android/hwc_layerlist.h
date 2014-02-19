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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_
#define MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_

#include "mir/graphics/android/fence.h"
#include "mir/geometry/rectangle.h"
#include "hwc_layers.h"
#include <hardware/hwcomposer.h>
#include <memory>
#include <vector>
#include <initializer_list>
#include <list>

namespace mir
{
namespace graphics
{

class Renderable;
class Buffer;

namespace android
{

class LayerListBase
{
public:
    std::weak_ptr<hwc_display_contents_1_t> native_list();
    NativeFence retirement_fence();

protected:
    LayerListBase(size_t initial_list_size);

    void update_representation(size_t needed_size); 
    std::list<HWCLayer> layers;

private:
    LayerListBase& operator=(LayerListBase const&) = delete;
    LayerListBase(LayerListBase const&) = delete;

    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
};

class LayerList : public LayerListBase
{
public:
    LayerList();
};

class FBTargetLayerList : public LayerListBase
{
public:
    FBTargetLayerList();
    void prepare_composition_layers(
        std::function<void(hwc_display_contents_1_t&)> const& prepare_fn,
        std::list<std::shared_ptr<graphics::Renderable>> const& list,
        std::function<void(Renderable const&)> const& render_fn);
    void prepare_default_layers( 
        std::function<void(hwc_display_contents_1_t&)> const& prepare_fn);

    bool needs_swapbuffers() const;
    void update_fences();
    void set_fb_target(Buffer const& buffer);
private:
    bool skip_layers_present{true};
    bool needs_gl_draw;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
