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

class HWCLayerBase;
typedef struct std::vector<std::shared_ptr<HWCLayerBase>> LayerList;
 
struct HWCRect : public hwc_rect_t
{
    HWCRect(); 
    HWCRect(geometry::Rectangle& rect);
};

struct HWCLayerBase : public hwc_layer_1
{
protected:
    HWCLayerBase();

    HWCRect visible_screen_rect;

    HWCLayerBase& operator=(HWCLayerBase const&) = delete;
    HWCLayerBase(HWCLayerBase const&) = delete;
};

struct HWCFBLayer : public HWCLayerBase
{
    HWCFBLayer(std::shared_ptr<compositor::NativeBufferHandle> const& native_buf,
               HWCRect& display_frame_rect);
};

class HWCLayerOrganizer
{
public:
    virtual ~HWCLayerOrganizer() {}
    virtual const LayerList& native_list() const = 0;
    virtual void set_fb_target(std::shared_ptr<compositor::Buffer> const&) = 0;

protected:
    HWCLayerOrganizer() = default;
    HWCLayerOrganizer& operator=(HWCLayerOrganizer const&) = delete;
    HWCLayerOrganizer(HWCLayerOrganizer const&) = delete;

};

class HWCLayerList : public HWCLayerOrganizer
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
