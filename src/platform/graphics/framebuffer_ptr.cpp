/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/graphics/framebuffer_ptr.h>
#include <mir/graphics/platform.h>

#include <utility>

namespace mg = mir::graphics;

mg::FramebufferDeleter::FramebufferDeleter() noexcept = default;

mg::FramebufferDeleter::FramebufferDeleter(std::move_only_function<void(Framebuffer*)> release) noexcept :
    release{std::move(release)}
{
}

mg::FramebufferDeleter::FramebufferDeleter(FramebufferDeleter&& from) noexcept = default;

auto mg::FramebufferDeleter::operator=(FramebufferDeleter&& from) noexcept -> FramebufferDeleter& = default;

mg::FramebufferDeleter::~FramebufferDeleter() = default;

void mg::FramebufferDeleter::operator()(Framebuffer* framebuffer)
{
    if (release)
    {
        std::exchange(release, nullptr)(framebuffer);
    }
    else
    {
        delete framebuffer;
    }
}
