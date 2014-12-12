/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_
#define MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_

#include <hardware/hwcomposer.h>
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{
struct HWCCallbacks;
class HwcWrapper
{
public:
    virtual ~HwcWrapper() = default;

    virtual void prepare(hwc_display_contents_1_t&) const = 0;
    virtual void set(hwc_display_contents_1_t&) const = 0;
    virtual void register_hooks(std::shared_ptr<HWCCallbacks> const& callbacks) = 0;
    virtual void vsync_signal_on() const = 0;
    virtual void vsync_signal_off() const = 0;
    virtual void display_on() const = 0;
    virtual void display_off() const = 0;

protected:
    HwcWrapper() = default;
    HwcWrapper& operator=(HwcWrapper const&) = delete;
    HwcWrapper(HwcWrapper const&) = delete;
};
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_ */
