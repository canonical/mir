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
#include "mir/geometry/displacement.h"
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

struct HwcLayerEntry
{
    HwcLayerEntry(HWCLayer && layer, bool needs_commit);
    HWCLayer layer;
    bool needs_commit;
};

class LayerList
{
public:
    LayerList(
        std::shared_ptr<LayerAdapter> const& layer_adapter,
        RenderableList const& renderlist,
        geometry::Displacement list_offset);
    void update_list(RenderableList const& renderlist, geometry::Displacement list_offset);

    std::list<HwcLayerEntry>::iterator begin();
    std::list<HwcLayerEntry>::iterator end();

    RenderableList rejected_renderables();
    void setup_fb(std::shared_ptr<Buffer> const& fb_target);
    bool needs_swapbuffers();
    void swap_occurred();

    hwc_display_contents_1_t* native_list();
    NativeFence retirement_fence();
private:
    LayerList& operator=(LayerList const&) = delete;
    LayerList(LayerList const&) = delete;

    RenderableList renderable_list;

    void update_list_mode(RenderableList const& renderlist);

    std::shared_ptr<LayerAdapter> const layer_adapter;
    std::list<HwcLayerEntry> layers;
    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
    enum Mode
    {
        no_extra_layers,
        skip_only,
        target_only,
        skip_and_target
    } mode;
    size_t additional_layers_for(Mode mode);
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
