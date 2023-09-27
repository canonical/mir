/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "display_buffer.h"

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;

mgv::DisplayBuffer::DisplayBuffer(std::shared_ptr<DisplayInterfaceProvider> provider)
    : provider{provider}
{

}

geom::Rectangle mgv::DisplayBuffer::view_area() const
{
    return area;
}

bool mgv::DisplayBuffer::overlay(std::vector<DisplayElement> const& renderlist)
{
    (void)renderlist;
    return false;
}

glm::mat2 mgv::DisplayBuffer::transformation() const
{
    return transform;
}

void mgv::DisplayBuffer::set_next_image(std::unique_ptr<Framebuffer> content)
{
    (void)content;
}

auto mgv::DisplayBuffer::display_provider() const -> std::shared_ptr<DisplayInterfaceProvider>
{
    return provider;
}

void mgv::DisplayBuffer::for_each_display_buffer(const std::function<void(mg::DisplayBuffer &)> &f)
{
    f(*this);
}

void mgv::DisplayBuffer::post()
{

}

std::chrono::milliseconds mgv::DisplayBuffer::recommended_sleep() const
{
    return std::chrono::milliseconds();
}

