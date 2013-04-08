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

#include <hardware/hwcomposer.h>
#include <memory>
#include <vector>

namespace mir
{
namespace compositor
{
class Buffer;
}
namespace graphics
{
namespace android
{

class LayerListManager
{
    //interface
};

typedef struct std::vector<std::shared_ptr<hwc_layer_1_t>> LayerList; 
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

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
