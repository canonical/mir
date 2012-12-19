/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/rendering_operator.h"
#include "mir/compositor/graphic_region.h"

#include <iostream>
namespace mc=mir::compositor;

mc::RenderingOperator::RenderingOperator(graphics::Renderer& renderer)
    : renderer(renderer)
{
}

void mc::RenderingOperator::operator()(graphics::Renderable& renderable)
{
    auto res = renderer.render(renderable);
    std::cout << res.use_count() << std::endl;
    resources.push_back(res);
}
