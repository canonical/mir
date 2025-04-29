/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "default_output_filter.h"
#include "mir/input/scene.h"

#include <memory>

namespace mg = mir::graphics;
namespace mi = mir::input;

mg::DefaultOutputFilter::DefaultOutputFilter(
    std::shared_ptr<mi::Scene> const& scene)
    : scene{scene}
{
}

std::string mg::DefaultOutputFilter::filter()
{
    return filter_;
}

void mg::DefaultOutputFilter::set_filter(std::string new_filter)
{
    filter_ = new_filter;
    scene->emit_scene_changed();
}

