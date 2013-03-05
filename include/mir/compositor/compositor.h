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

#ifndef MIR_COMPOSITOR_COMPOSITOR_H_
#define MIR_COMPOSITOR_COMPOSITOR_H_

#include "mir/compositor/drawer.h"
#include "mir/compositor/render_view.h"
#include "mir/shell/shell.h"

#include <memory>

namespace mir
{
namespace graphics
{

class Renderer;

}

///  Compositing. Combining renderables into a display image.
namespace compositor
{

class Compositor : public Drawer
{
public:
    explicit Compositor(
        std::shared_ptr<RenderView> const& render_view,
        const std::shared_ptr<graphics::Renderer>& renderer);

    virtual ~Compositor();

    virtual void render(graphics::Display* display);

    void set_shell(std::shared_ptr<mir::Shell> s);
    bool start_shell();
    void stop_shell();
    std::shared_ptr<mir::Shell> current_shell() const;

private:
    std::shared_ptr<RenderView> const render_view;
    std::shared_ptr<graphics::Renderer> const renderer;

    std::shared_ptr<mir::Shell> shell;
    bool                        shell_running;
};


}
}

#endif /* MIR_COMPOSITOR_COMPOSITOR_H_ */
