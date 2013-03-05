/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/compositor.h"

#include "mir/compositor/rendering_operator.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/renderer.h"
#include "mir/shell/nullshell.h"

#include <cassert>
#include <functional>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::Compositor::Compositor(
    std::shared_ptr<RenderView> const& view,
    const std::shared_ptr<mg::Renderer>& renderer)
    : render_view(view),
      renderer(renderer),
      shell_running(false)
{
    assert(render_view);
    assert(renderer);
}

mc::Compositor::~Compositor()
{
    if (shell_running)
        stop_shell();
}

namespace
{

struct FilterForVisibleRenderablesInRegion : public mc::FilterForRenderables
{
    FilterForVisibleRenderablesInRegion(mir::geometry::Rectangle enclosing_region)
        : enclosing_region(enclosing_region)
    {
    }
    bool operator()(mg::Renderable& renderable)
    {
        return !renderable.hidden();
    }
    mir::geometry::Rectangle& enclosing_region;
};

}

void mc::Compositor::render(graphics::Display* display)
{
    display->for_each_display_buffer([&](mg::DisplayBuffer& buffer)
    {
        RenderingOperator applicator(*renderer);
        FilterForVisibleRenderablesInRegion selector(buffer.view_area());

        buffer.make_current();
        buffer.clear();

        render_view->for_each_if(selector, applicator);

        buffer.post_update();
    });
}

// TODO: Write unit tests for these

void mc::Compositor::set_shell(std::shared_ptr<mir::Shell> s)
{
    stop_shell();
    shell = s;
}

bool mc::Compositor::start_shell()
{
    if (!shell)
        set_shell(std::make_shared<mir::shell::NullShell>());

    shell_running = shell->start();
    return shell_running;
}

void mc::Compositor::stop_shell()
{
    if (shell_running)
    {
        shell->stop();
        shell_running = false;
    }
}

std::shared_ptr<mir::Shell> mc::Compositor::current_shell() const
{
    return shell;
}
