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

/* this is a partitioned list. renderlist makes up the first renderlist.size() elements
   of the list, and there are additional_layers added to the end. 
   std::distance(begin(), additional_layers_begin()) == renderlist.size() 
   std::distance(additional_layers_begin(), end()) == additional_layers
   std::distance(begin(), end()) == renderlist.size() + additional_layers 
*/ 
class LayerList
{
public:
    LayerList(RenderableList const& renderlist, size_t additional_layers);
    bool update_list_and_check_if_changed(
        RenderableList const& renderlist,
        size_t additional_layers);

    std::list<HWCLayer>::iterator begin();
    std::list<HWCLayer>::iterator additional_layers_begin();
    std::list<HWCLayer>::iterator end();

    std::weak_ptr<hwc_display_contents_1_t> native_list();
    NativeFence retirement_fence();
private:
    LayerList& operator=(LayerList const&) = delete;
    LayerList(LayerList const&) = delete;

    std::list<HWCLayer> layers;
    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
    std::list<HWCLayer>::iterator first_additional_layer;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
