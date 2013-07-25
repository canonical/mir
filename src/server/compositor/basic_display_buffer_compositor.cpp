/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/basic_display_buffer_compositor.h"

#include "mir/graphics/display_buffer.h"

#include <vector>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BasicDisplayBufferCompositor::BasicDisplayBufferCompositor(
    mg::DisplayBuffer& display_buffer)
    : display_buffer(display_buffer)
{
}

// TODO I'm not clear why this code is in the DisplayBufferCompositor hierarchy.
//      (As opposed to the call site calling compose directly.)
//      But changing that breaks RecordingDisplayBufferCompositor - which is too
//      much churn for this refactoring.
void mc::BasicDisplayBufferCompositor::composite()
{
    // preserves buffers used in rendering until after post_update()
    std::vector<std::shared_ptr<void>> saved_resources;
    auto save_resource = [&](std::shared_ptr<void> const& r) { saved_resources.push_back(r); };

    display_buffer.make_current();

    compose(display_buffer.view_area(), save_resource);

    display_buffer.post_update();
}
