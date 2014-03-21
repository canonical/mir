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

#include <vector>
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
        unsigned long frameno);
    ~RenderingOperator() = default;

    void operator()(graphics::Renderable const&);
    bool uncomposited_buffers() const;
    bool anything_was_rendered() const;

private:
    Renderer& renderer;
    std::vector<std::shared_ptr<void>> saved_resources;
    unsigned long const frameno;
    bool uncomposited_buffers_;
};

}
}
#endif /* MIR_COMPOSITOR_RENDERING_OPERATOR_H_ */
