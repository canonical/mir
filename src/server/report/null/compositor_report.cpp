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

#include "compositor_report.h"

namespace mrn = mir::report::null;

void mrn::CompositorReport::added_display(int, int, int, int, SubCompositorId)
{
}

void mrn::CompositorReport::began_frame(SubCompositorId)
{
}

void mrn::CompositorReport::renderables_in_frame(SubCompositorId, mir::graphics::RenderableList const&)
{
}

void mrn::CompositorReport::rendered_frame(SubCompositorId)
{
}

void mrn::CompositorReport::finished_frame(SubCompositorId)
{
}

void mrn::CompositorReport::started()
{
}

void mrn::CompositorReport::stopped()
{
}

void mrn::CompositorReport::scheduled()
{
}
