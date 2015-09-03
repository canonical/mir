/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "display_group.h"

namespace mg = mir::graphics;
namespace mgx = mg::X;

mgx::DisplayGroup::DisplayGroup(std::unique_ptr<mgx::DisplayBuffer> primary_buffer)
{
    display_buffer = std::move(primary_buffer);
}

void mgx::DisplayGroup::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*display_buffer);
}

void mgx::DisplayGroup::post()
{
}

std::chrono::milliseconds mgx::DisplayGroup::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}

void mgx::DisplayGroup::set_orientation(MirOrientation orientation)
{
    display_buffer->set_orientation(orientation);
}
