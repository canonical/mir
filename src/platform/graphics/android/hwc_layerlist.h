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

#include "mir/graphics/renderable.h"
#include "mir/graphics/android/fence.h"
#include "mir/geometry/rectangle.h"
#include "hwc_layers.h"
#include <hardware/hwcomposer.h>
#include <memory>
#include <vector>
#include <initializer_list>
#include <list>

namespace
{
bool const use_fb_target = true;
bool const do_not_use_fb_target = false;
}

namespace mir
{
namespace graphics
{

class Renderable;
class NativeBuffer;
class Buffer;

namespace android
{

class LayerList
{
public:
    LayerList(bool use_fb_target);
    void set_composition_layers(std::list<std::shared_ptr<graphics::Renderable>> const& list);
    void reset_composition_layers(); 
    void with_native_list(std::function<void(hwc_display_contents_1_t&)> const& fn);

    void set_fb_target(NativeBuffer const&);
    NativeFence fb_target_fence();
    NativeFence retirement_fence();

private:
    LayerList& operator=(LayerList const&) = delete;
    LayerList(LayerList const&) = delete;

    void update_representation();

    bool composition_layers_present;
    bool const fb_target_present;

    std::shared_ptr<ForceGLLayer> skip_layer;
    std::shared_ptr<FramebufferLayer> fb_target_layer;

    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
    std::list<std::shared_ptr<HWCLayer>> layers;

    NativeFence retire_fence;
    NativeFence fb_fence;

    int fake_egl_values;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
