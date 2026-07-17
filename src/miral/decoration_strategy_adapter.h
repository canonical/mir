/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_DECORATION_STRATEGY_ADAPTER_H_
#define MIRAL_DECORATION_STRATEGY_ADAPTER_H_

#include <miral/decoration_strategy.h>

#include <mir/scene/surface.h>
#include <mir/shell/decoration/decoration_strategy.h>

#include <memory>
#include <optional>

namespace miral
{

class RendererAdapter : public mir::shell::decoration::RendererStrategy
{
public:
    RendererAdapter(
        std::shared_ptr<miral::DecorationStrategy> user,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator);

    void update_state(mir::shell::decoration::WindowState const& ws, mir::shell::decoration::InputState const& input)
        override;

    auto render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;

private:
    auto render_from_user(
        std::optional<DecorationSurface> (DecorationStrategy::*render)(
            DecorationStrategy::WindowState const&,
            DecorationStrategy::InputState const&,
            DecorationBuffers&)) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;

    std::shared_ptr<miral::DecorationStrategy> user;
    DecorationBuffers buffers;
    miral::DecorationStrategy::WindowState window_state;
    miral::DecorationStrategy::InputState input_state;
};

class DecorationStrategyAdapter : public mir::shell::decoration::DecorationStrategy
{
public:
    DecorationStrategyAdapter(
        std::shared_ptr<miral::DecorationStrategy> user,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> alloc);

    auto render_strategy() const -> std::unique_ptr<mir::shell::decoration::RendererStrategy> override;
    auto button_placement(unsigned n, mir::shell::decoration::WindowState const& ws) const
        -> mir::geometry::Rectangle override;
    auto compute_size_with_decorations(mir::geometry::Size content_size, MirWindowType type, MirWindowState state) const
        -> mir::geometry::Size override;
    auto new_window_state(std::shared_ptr<mir::scene::Surface> const& surface, float scale) const
        -> std::unique_ptr<mir::shell::decoration::WindowState> override;
    auto buffer_format() const -> MirPixelFormat override;
    auto resize_corner_input_size() const -> mir::geometry::Size override;

private:
    std::shared_ptr<miral::DecorationStrategy> user;
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator;
};

}

#endif // MIRAL_DECORATION_STRATEGY_ADAPTER_H_
