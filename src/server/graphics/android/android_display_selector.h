/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_SELECTOR_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_SELECTOR_H_

#include "display_selector.h"

namespace mir
{
namespace graphics
{

class Display;

namespace android
{

class FBFactory;

class AndroidDisplaySelector : public DisplaySelector
{
public:
    AndroidDisplaySelector(std::shared_ptr<FBFactory> const& factory); 
    std::shared_ptr<Display> primary_display();

private:
    std::shared_ptr<FBFactory> fb_factory;
    std::shared_ptr<Display> primary_hwc_display;
    std::function<std::shared_ptr<Display>()> allocate_primary_fb;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_SELECTOR_H_ */
