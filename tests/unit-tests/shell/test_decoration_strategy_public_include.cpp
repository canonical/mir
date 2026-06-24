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

#include <miral/decoration_strategy.h>
#include <miral/decorations.h>
#include <miral/runner.h>

#include <mir/test/doubles/stub_buffer_allocator.h>

#include <gtest/gtest.h>

#include <memory>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
// Minimal concrete DecorationStrategy for construction and basic path tests.
// Implements all pure virtuals with dummies (no gmock per requirements).
struct MinimalStrategy : miral::DecorationStrategy
{
    auto compute_size_with_decorations(geom::Size, MirWindowType, MirWindowState) const -> geom::Size override
    {
        return {};
    }

    auto button_placement(unsigned, WindowState const&) const -> geom::Rectangle override { return {}; }

    void update_state(WindowState const&, InputState const&) override {}

    auto render_titlebar() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }

    auto render_left_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }

    auto render_right_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }

    auto render_bottom_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }

    auto resize_corner_input_size() const -> geom::Size override { return {}; }
};

// Basic MinimalDecorationStrategy test skeleton (instantiates via CustomDecorations::strategy).
struct MinimalDecorationStrategy : MinimalStrategy {
    auto titlebar_height() const -> geom::Height override { return geom::Height{42}; }
};

// Records set_buffer_allocator and exercises create_software_buffer post-set (no fatal).
struct RecordingStrategy : miral::DecorationStrategy
{
    bool set_called{false};
    std::shared_ptr<mg::GraphicBufferAllocator> received;
    bool render_title_text_called{false};

    void set_buffer_allocator(std::shared_ptr<mg::GraphicBufferAllocator> const& a) override
    {
        set_called = true;
        received = a;
        miral::DecorationStrategy::set_buffer_allocator(a);
    }

    auto compute_size_with_decorations(geom::Size, MirWindowType, MirWindowState) const -> geom::Size override
    {
        return {};
    }
    auto button_placement(unsigned, WindowState const&) const -> geom::Rectangle override { return {}; }
    void update_state(WindowState const&, InputState const&) override {}

    auto render_titlebar() -> std::optional<std::shared_ptr<mg::Buffer>> override
    {
        if (set_called)
        {
            // Exercise the path: must not hit "no allocator" fatal here in unit test.
            auto buf = create_software_buffer(geom::Size{8, 8});
            (void)buf;
        }
        return std::nullopt;
    }

    auto render_left_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }
    auto render_right_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }
    auto render_bottom_border() -> std::optional<std::shared_ptr<mg::Buffer>> override { return std::nullopt; }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }
    auto resize_corner_input_size() const -> geom::Size override { return {}; }
    void render_title_text(uint32_t*,geom::Size,std::string const&,geom::Point,geom::Height,uint32_t) override { render_title_text_called=true; }
};
}

// pack_color produces the memory byte layout (b,g,r,a) for ARGB on both LE and BE.
TEST(DecorationStrategyPublic, pack_color_memory_layout)
{
    auto const packed = miral::pack_color(0x12, 0x34, 0x56, 0x78);
    auto const p = reinterpret_cast<uint8_t const*>(&packed);
    EXPECT_EQ(0x78u, p[0]); // blue
    EXPECT_EQ(0x56u, p[1]); // green
    EXPECT_EQ(0x34u, p[2]); // red
    EXPECT_EQ(0x12u, p[3]); // alpha
}

// CustomDecorations::strategy(valid) succeeds; resulting can be passed to MirRunner::run_with
// at construction time without immediate fatal (pre-init adapter not yet).
TEST(DecorationStrategyPublic, custom_decorations_construction_succeeds)
{
    auto const strategy = std::make_shared<MinimalDecorationStrategy>();
    auto const custom = miral::CustomDecorations::strategy(strategy);
    EXPECT_EQ(geom::Height{42}, strategy->titlebar_height());
    strategy->render_title_text(nullptr, {}, "", {}, {}, 0); // exercises default render_title_text path + internal Text::instance() w/o crash
    // Construction of MirRunner + passing the custom (as would be in run_with({...})) is fine.
    int argc = 1;
    char const* argv[] = {"test"};
    miral::MirRunner runner{argc, argv};
    (void)custom; // would be e.g. runner.run_with({custom, ...});
    (void)runner;
}

// Manual recording subclass verifies set_buffer_allocator called and create_software_buffer
// invocable after without fatal (using stub allocator from test tree).
TEST(DecorationStrategyPublic, set_buffer_allocator_and_create_path)
{
    auto const rec = std::make_shared<RecordingStrategy>();
    auto const alloc = std::make_shared<mtd::StubBufferAllocator>();
    rec->set_buffer_allocator(alloc);
    EXPECT_TRUE(rec->set_called);
    EXPECT_TRUE(rec->received.get() != nullptr);
    // Call render which internally does create_software_buffer (exercises lock success).
    auto const maybe_buf = rec->render_titlebar();
    (void)maybe_buf;
}

TEST(DecorationStrategyPublic, render_title_text_is_called_on_custom_strategy)
{
    auto const rec = std::make_shared<RecordingStrategy>();
    std::shared_ptr<miral::DecorationStrategy> base = rec;
    auto const custom = miral::CustomDecorations::strategy(rec);
    (void)custom;
    base->render_title_text(nullptr, {}, "", {}, {}, 0);
    EXPECT_TRUE(rec->render_title_text_called);
}

// Exercise title text through custom strategy that does not override it (uses default
// which calls the internal free render_title_text + FT impl or fallback). Checks no crash.
TEST(DecorationStrategyPublic, render_title_text_default_on_custom_strategy)
{
    auto const s = std::make_shared<MinimalDecorationStrategy>();
    auto const custom = miral::CustomDecorations::strategy(s);
    (void)custom;
    s->render_title_text(nullptr, {}, "hi", {}, {}, 0); // no crash
}
