/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/multi_display_clock.h"

using namespace mir::graphics;

Frame MultiDisplayClock::last_frame() const
{
    return {};
}

void MultiDisplayClock::on_next_frame(FrameCallback const& f)
{
    (void)f;
}

void MultiDisplayClock::add_child_clock(std::weak_ptr<DisplayClock> w)
{
    auto dc = w.lock();
    if (dc)
    {
    }
}

void MultiDisplayClock::synchronize()
{
}

void MultiDisplayClock::on_child_frame(Frame const& f)
{
    (void)f;
}

