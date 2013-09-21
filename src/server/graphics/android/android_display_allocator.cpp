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
#include "android_framebuffer_window.h"
#include "gpu_android_display_buffer_factory.h"
#include "hwc_android_display_buffer_factory.h"
#include "hwc_device.h"
#include "fb_device.h"

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

std::shared_ptr<mga::AndroidDisplay> mga::AndroidDisplayAllocator::create_gpu_display(
    std::shared_ptr<ANativeWindow> const& native_window,
    std::shared_ptr<DisplaySupportProvider> const& display_provider,
    std::shared_ptr<DisplayReport> const& display_report) const
{
    auto window = std::make_shared<mga::AndroidFramebufferWindow>(native_window);
    auto db_factory = std::make_shared<mga::GPUAndroidDisplayBufferFactory>();
    return std::make_shared<AndroidDisplay>(window, db_factory, display_provider, display_report);
}

std::shared_ptr<mga::AndroidDisplay> mga::AndroidDisplayAllocator::create_hwc_display(
    std::shared_ptr<HWCDevice> const& hwc_device,
    std::shared_ptr<ANativeWindow> const& anw,
    std::shared_ptr<DisplayReport> const& display_report) const
{
    auto window = std::make_shared<mga::AndroidFramebufferWindow>(anw);
    auto db_factory = std::make_shared<mga::HWCAndroidDisplayBufferFactory>(hwc_device);
    return std::make_shared<AndroidDisplay>(window, db_factory, hwc_device, display_report);
}
