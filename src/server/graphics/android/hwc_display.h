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

#ifndef MIR_GRAPHICS_ANDROID_HWC_DISPLAY_H_
#define MIR_GRAPHICS_ANDROID_HWC_DISPLAY_H_

#include <memory>
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "android_display.h"
#include "hwc_device.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{

class HWCDevice;
class AndroidFramebufferWindowQuery;

class HWCDisplay : public virtual Display,
                   public virtual DisplayBuffer,
                   private AndroidDisplay
{
public:
    HWCDisplay(std::shared_ptr<AndroidFramebufferWindowQuery> const& fb_window,
               std::shared_ptr<HWCDevice> const& hwc_device,
               std::shared_ptr<DisplayReport> const&);

    geometry::Rectangle view_area() const;
    void post_update();
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f);
    std::shared_ptr<DisplayConfiguration> configuration();
    void make_current();

private:
    std::shared_ptr<HWCDevice> hwc_device;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_HWC_DISPLAY_H_ */
