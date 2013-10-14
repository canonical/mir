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

#ifndef MIR_GRAPHICS_ANDROID_RESOURCE_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_RESOURCE_FACTORY_H_

#include "display_resource_factory.h"

#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{

class GraphicBufferAllocator;
class FBSwapper;

class ResourceFactory : public DisplayResourceFactory
{
public:
    explicit ResourceFactory(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator);

    std::shared_ptr<DisplaySupportProvider> create_fb_device() const;

    std::shared_ptr<DisplaySupportProvider> create_hwc_1_1(
        std::shared_ptr<hwc_composer_device_1> const& hwc_device,
        std::shared_ptr<DisplaySupportProvider> const& fb_device) const;

    std::shared_ptr<DisplaySupportProvider> create_hwc_1_0(
        std::shared_ptr<hwc_composer_device_1> const& hwc_device,
        std::shared_ptr<DisplaySupportProvider> const& fb_device) const;

    std::shared_ptr<graphics::Display> create_display(
        std::shared_ptr<DisplaySupportProvider> const& support_provider,
        std::shared_ptr<graphics::DisplayReport> const& report) const;

private:
    std::shared_ptr<GraphicBufferAllocator> const buffer_allocator;

    virtual std::vector<std::shared_ptr<graphics::Buffer>> create_buffers(
        std::shared_ptr<DisplaySupportProvider> const& info_provider) const;

    virtual std::shared_ptr<FBSwapper> create_swapper(
        std::vector<std::shared_ptr<graphics::Buffer>> const& buffers) const;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DEFAULT_FRAMEBUFFER_FACTORY_H_ */
