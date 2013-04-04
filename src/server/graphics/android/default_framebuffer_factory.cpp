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

#include "default_framebuffer_factory.h"

namespace mga=mir::graphics::android;

mga::DefaultFramebufferFactory::DefaultFramebufferFactory(std::shared_ptr<GraphicAllocAdaptor> const& /*buffer_allocator*/)
{
}

std::shared_ptr<ANativeWindow> mga::DefaultFramebufferFactory::create_fb_native_window(std::shared_ptr<DisplayInfoProvider> const&)
{
    return std::make_shared<ANativeWindow>();
}
