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

#include <miral/custom_decorations.h>
#include <miral/decorations.h>
#include <miral/runner.h>

#include "decoration_strategy_adapter.h"

#include <mir/fatal.h>
#include <mir/test/death.h>
#include <mir/test/doubles/stub_buffer_allocator.h>

#include <gtest/gtest.h>

#include <memory>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
struct MinimalStrategy : miral::DecorationStrategy
{
    auto compute_size_with_decorations(geom::Size, MirWindowType, MirWindowState) const -> geom::Size override
    {
        return {};
    }

    auto button_placement(unsigned, WindowState const&) const -> geom::Rectangle override { return {}; }

    auto render_titlebar(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }

    auto render_left_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }

    auto render_right_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }

    auto render_bottom_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }

    auto resize_corner_input_size() const -> geom::Size override { return {}; }
};

struct TitledStrategy : MinimalStrategy
{
    auto titlebar_height() const -> geom::Height override { return geom::Height{42}; }
};

struct RecordingStrategy : miral::DecorationStrategy
{
    bool render_title_text_called{false};

    auto compute_size_with_decorations(geom::Size, MirWindowType, MirWindowState) const -> geom::Size override
    {
        return {};
    }
    auto button_placement(unsigned, WindowState const&) const -> geom::Rectangle override { return {}; }

    auto render_titlebar(WindowState const&, InputState const&, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface> override
    {
        auto pixels = buffers.create_mapping(geom::Size{8, 8}, mir_pixel_format_argb_8888);
        return std::move(pixels.surface);
    }

    auto render_left_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }
    auto render_right_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }
    auto render_bottom_border(WindowState const&, InputState const&, miral::DecorationBuffers&)
        -> std::optional<miral::DecorationSurface> override
    {
        return std::nullopt;
    }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }
    auto resize_corner_input_size() const -> geom::Size override { return {}; }

    void render_title_text(uint32_t*, geom::Size, std::string const&, geom::Point, geom::Height, uint32_t) override
    {
        render_title_text_called = true;
    }
};
} // namespace

TEST(DecorationStrategyPublic, pack_color_memory_layout)
{
    auto const packed = miral::pack_color(0x12, 0x34, 0x56, 0x78);
    auto const p = reinterpret_cast<uint8_t const*>(&packed);
    EXPECT_EQ(0x78u, p[0]);
    EXPECT_EQ(0x56u, p[1]);
    EXPECT_EQ(0x34u, p[2]);
    EXPECT_EQ(0x12u, p[3]);
}

TEST(DecorationStrategyPublic, custom_decorations_construction_succeeds)
{
    auto const strategy = std::make_shared<TitledStrategy>();
    miral::CustomDecorations const custom{strategy};
    EXPECT_EQ(geom::Height{42}, strategy->titlebar_height());
    strategy->render_title_text(nullptr, {}, "", {}, {}, 0);

    int argc = 1;
    char const* argv[] = {"test"};
    miral::MirRunner runner{argc, argv};
    (void)custom;
    (void)runner;
}

TEST(DecorationStrategyPublic, custom_decorations_null_strategy_fails)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};
    MIR_EXPECT_DEATH(miral::CustomDecorations{nullptr}, "CustomDecorations: null DecorationStrategy");
}

TEST(DecorationStrategyPublic, adapter_produces_renderer_strategy)
{
    auto const alloc = std::make_shared<mtd::StubBufferAllocator>();
    auto const rec = std::make_shared<RecordingStrategy>();
    miral::DecorationStrategyAdapter adapter{rec, alloc};
    EXPECT_TRUE(adapter.render_strategy());
}

TEST(DecorationStrategyPublic, create_mapping_returns_surface_from_render)
{
    auto const alloc = std::make_shared<mtd::StubBufferAllocator>();
    auto const rec = std::make_shared<RecordingStrategy>();
    miral::RendererAdapter renderer{rec, alloc};
    EXPECT_TRUE(renderer.render_titlebar());
}

TEST(DecorationStrategyPublic, render_title_text_override)
{
    auto const rec = std::make_shared<RecordingStrategy>();
    miral::CustomDecorations const custom{rec};
    (void)custom;
    rec->render_title_text(nullptr, {}, "", {}, {}, 0);
    EXPECT_TRUE(rec->render_title_text_called);
}
