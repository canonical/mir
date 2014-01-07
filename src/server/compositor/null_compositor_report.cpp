/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/compositor/compositor_report.h"

using namespace mir::compositor;

void NullCompositorReport::added_display(int, int, int, int, SubCompositorId)
{
}

void NullCompositorReport::began_frame(SubCompositorId)
{
}

void NullCompositorReport::finished_frame(bool, SubCompositorId)
{
}

void NullCompositorReport::started()
{
}

void NullCompositorReport::stopped()
{
}

void NullCompositorReport::scheduled()
{
}
