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

#include "android_display_allocator.h"
#include "android_display.h"
#include "hwc_display.h"
#include "android_framebuffer_window.h"

#include <ui/FramebufferNativeWindow.h>

namespace mga=mir::graphics::android;

std::shared_ptr<mga::AndroidDisplay> mga::AndroidDisplayAllocator::create_gpu_display() const
{
    auto native_window = std::make_shared< ::android::FramebufferNativeWindow>();
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (native_window);
    return std::make_shared<AndroidDisplay>(window);
}

std::shared_ptr<mga::HWCDisplay> mga::AndroidDisplayAllocator::create_hwc_display(std::shared_ptr<HWCDevice> const& hwc_device) const
{
    auto native_window = std::make_shared< ::android::FramebufferNativeWindow>();
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (native_window);
    return std::make_shared<HWCDisplay>(window, hwc_device);
}
