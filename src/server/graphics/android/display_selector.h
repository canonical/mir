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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_SELECTOR_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_SELECTOR_H_

#include <memory>

namespace mir
{
namespace graphics
{

class Display;
namespace android
{

class DisplaySelector
{
public:
    virtual std::shared_ptr<Display> primary_display() = 0; 
    virtual ~DisplaySelector() {}

protected:
    DisplaySelector() = default;
    DisplaySelector& operator=(DisplaySelector const&) = delete;
    DisplaySelector(DisplaySelector const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_SELECTOR_H_ */
