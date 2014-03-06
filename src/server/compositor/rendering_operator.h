/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_COMPOSITOR_RENDERING_OPERATOR_H_
#define MIR_COMPOSITOR_RENDERING_OPERATOR_H_

#include "mir/compositor/renderer.h"
#include "mir/compositor/scene.h"

#include <functional>
#include <memory>

namespace mir
{
namespace compositor
{

class RenderingOperator : public OperatorForScene
{
public:
    explicit RenderingOperator(
        Renderer& renderer,
        std::function<void(std::shared_ptr<void> const&)> save_resource,
        unsigned long frameno,
        bool& uncomposited_buffers);
    ~RenderingOperator() = default;

    void operator()(graphics::Renderable const&);

private:
    Renderer& renderer;
    std::function<void(std::shared_ptr<void> const&)> save_resource;
    unsigned long const frameno;
    bool& uncomposited_buffers;
};

}
}
#endif /* MIR_COMPOSITOR_RENDERING_OPERATOR_H_ */
