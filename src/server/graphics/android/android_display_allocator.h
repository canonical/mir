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

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_ALLOCATOR_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_ALLOCATOR_H_

#include "display_allocator.h"

namespace mir
{
namespace graphics
{
namespace android
{

class AndroidDisplayAllocator : public DisplayAllocator
{
public:
    std::shared_ptr<AndroidDisplay> create_gpu_display(
        std::shared_ptr<ANativeWindow> const&,
        std::shared_ptr<DisplaySupportProvider> const&,
        std::shared_ptr<DisplayReport> const&) const;

    std::shared_ptr<AndroidDisplay> create_hwc_display(
        std::shared_ptr<HWCDevice> const&,
        std::shared_ptr<ANativeWindow> const&,
        std::shared_ptr<DisplayReport> const&) const;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_ALLOCATOR_H_ */
