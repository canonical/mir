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
class Buffer;

namespace android
{
 
struct HWCRect
{
    HWCRect(); 
    HWCRect(geometry::Rectangle& rect);

    operator hwc_rect_t const& () const { return self; }
    operator hwc_rect_t& () { return self; }
private:
    hwc_rect_t self;
};

struct HWCDefaultLayer
{
    HWCDefaultLayer(std::initializer_list<HWCRect> list);
    ~HWCDefaultLayer();

    operator hwc_layer_1 const& () const { return self; }
    operator hwc_layer_1& () { return self; }

protected:
    HWCDefaultLayer& operator=(HWCDefaultLayer const&) = delete;
    HWCDefaultLayer(HWCDefaultLayer const&) = delete;

    hwc_layer_1 self;
};

struct HWCFBLayer : public HWCDefaultLayer
{
    HWCFBLayer();
    HWCFBLayer(buffer_handle_t native_buf,
               HWCRect display_frame_rect);
};

class HWCLayerList
{
public:
    virtual ~HWCLayerList() = default;

    virtual hwc_display_contents_1_t* native_list() const = 0;
    virtual void set_fb_target(std::shared_ptr<Buffer> const&) = 0;

protected:
    HWCLayerList() = default;
    HWCLayerList& operator=(HWCLayerList const&) = delete;
    HWCLayerList(HWCLayerList const&) = delete;

};

class LayerList : public HWCLayerList
{
public:
    LayerList();

    void set_fb_target(std::shared_ptr<Buffer> const&);
    hwc_display_contents_1_t* native_list() const;

private:
    std::vector<std::shared_ptr<HWCDefaultLayer>> layer_list;
    void update_list();

    static size_t const fb_position = 0u;
    std::shared_ptr<hwc_display_contents_1_t> hwc_representation;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_LAYERLIST_H_ */
