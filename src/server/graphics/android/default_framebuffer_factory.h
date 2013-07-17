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

#ifndef MIR_GRAPHICS_ANDROID_DEFAULT_FRAMEBUFFER_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_DEFAULT_FRAMEBUFFER_FACTORY_H_

#include "framebuffer_factory.h"

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

class DefaultFramebufferFactory : public FramebufferFactory
{
public:
    explicit DefaultFramebufferFactory(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator);
    std::shared_ptr<ANativeWindow> create_fb_native_window(std::shared_ptr<DisplaySupportProvider> const&) const;
    std::shared_ptr<DisplaySupportProvider> create_fb_device() const; 
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
