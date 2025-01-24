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

#ifndef MIR_SHELL_DECORATION_RENDERER_H_
#define MIR_SHELL_DECORATION_RENDERER_H_

#include "decoration_strategy.h"
#include "input.h"

#include "mir/geometry/rectangle.h"


#include <memory>
#include <map>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
}
namespace shell
{
namespace decoration
{
class WindowState;
class InputState;
struct StaticGeometry;

class Renderer : BufferMaker
{
public:
    Renderer(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::unique_ptr<RendererStrategy> strategy);

    void update_state(WindowState const& window_state, InputState const& input_state);
    auto render_titlebar() -> std::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_left_border() -> std::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_right_border() -> std::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_bottom_border() -> std::optional<std::shared_ptr<graphics::Buffer>>;

private:
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::unique_ptr<RendererStrategy> const strategy;

    auto make_buffer(const MirPixelFormat format, const geometry::Size size,
        Pixel const* const pixels) const -> std::optional<std::shared_ptr<graphics::Buffer>> override;
};
}
}
}

#endif // MIR_SHELL_DECORATION_RENDERER_H_
