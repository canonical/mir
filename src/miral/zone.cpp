/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "miral/zone.h"

#include <atomic>

class miral::Zone::Self
{
public:
    struct IdTag;

    Self(mir::IntWrapper<IdTag> id, Rectangle const& extents)
        : id{id}, extents{extents}
    {
    }

    auto operator=(Self const&) -> Self& = default;

    mir::IntWrapper<IdTag> id;
    Rectangle extents;

    static auto new_id() -> mir::IntWrapper<IdTag>
    {
        static std::atomic<int> next_id{1};
        return mir::IntWrapper<IdTag>{next_id++};
    }
};

miral::Zone::Zone(Rectangle const& extents)
    : self{std::make_unique<Self>(Self::new_id(), extents)}
{
}

miral::Zone::Zone(Zone const& other)
    : self{std::make_unique<Self>(other.self->id, other.self->extents)}
{
}

miral::Zone& miral::Zone::operator=(Zone const& other)
{
    *self = *other.self;
    return *this;
}

miral::Zone::~Zone() = default;

auto miral::Zone::operator==(Zone const& other) const -> bool
{
    return self->id == other.self->id
        && self->extents == other.self->extents;
}

auto miral::Zone::is_same_zone(Zone const& other) const -> bool
{
    return self->id == other.self->id;
}

auto miral::Zone::extents() const -> Rectangle
{
    return self->extents;
}

void miral::Zone::extents(Rectangle const& extents)
{
    self->extents = extents;
}
