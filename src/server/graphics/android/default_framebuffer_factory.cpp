/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/mir_native_window.h"
#include "default_framebuffer_factory.h"
#include "display_info_provider.h"
#include "graphic_buffer_allocator.h"
#include "server_render_window.h"
#include "fb_swapper.h"

#include <vector>

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace mcla=mir::client::android;

mga::DefaultFramebufferFactory::DefaultFramebufferFactory(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator)
    : buffer_allocator(buffer_allocator)
{
}

std::shared_ptr<ANativeWindow> mga::DefaultFramebufferFactory::create_fb_native_window(std::shared_ptr<DisplayInfoProvider> const& info_provider)
{
    auto size = info_provider->display_size();
    auto pf = info_provider->display_format();
    auto num_framebuffers = info_provider->number_of_framebuffers_available();
    
    std::vector<std::shared_ptr<mc::Buffer>> buffers; 
    for( auto i = 0u; i < num_framebuffers; ++i)
    {
        buffers.push_back(buffer_allocator->alloc_buffer_platform(size, pf, mga::BufferUsage::use_framebuffer_gles));
    }

    auto swapper = std::make_shared<mga::FBSwapper>(buffers);
    auto interpreter = std::make_shared<mga::ServerRenderWindow>(swapper, info_provider);
    return std::make_shared<mcla::MirNativeWindow>(interpreter); 
}
