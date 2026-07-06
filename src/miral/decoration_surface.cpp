/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <miral/decoration_surface.h>

#include <mir/graphics/buffer.h>

#include <utility>

namespace miral
{

struct DecorationSurface::Self
{
    std::shared_ptr<mir::graphics::Buffer> buffer;
};

DecorationSurface::DecorationSurface(std::shared_ptr<mir::graphics::Buffer> buffer) :
    self{std::make_unique<Self>()}
{
    self->buffer = std::move(buffer);
}

auto DecorationSurface::take_buffer() -> std::shared_ptr<mir::graphics::Buffer>
{
    return std::move(self->buffer);
}

DecorationSurface::~DecorationSurface() = default;

DecorationSurface::DecorationSurface(DecorationSurface&&) noexcept = default;

auto DecorationSurface::operator=(DecorationSurface&&) noexcept -> DecorationSurface& = default;

}
