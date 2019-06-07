/*
 * Copyright © 2016-2019 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_TEST_WINDOW_MANAGER_TOOLS_H
#define MIRAL_TEST_WINDOW_MANAGER_TOOLS_H

#include "basic_window_manager.h"

#include <miral/canonical_window_manager.h>
#include <miral/output.h>

#include <mir/scene/surface_creation_parameters.h>
#include <mir/shell/display_layout.h>
#include <mir/shell/focus_controller.h>
#include <mir/shell/persistent_surface_store.h>
#include "mir/graphics/display_configuration_observer.h"

#include <mir/test/doubles/stub_session.h>
#include <mir/test/doubles/stub_surface.h>
#include "mir/test/doubles/mock_display_configuration.h"
#include <mir/test/fake_shared.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>

struct StubFocusController : mir::shell::FocusController
{
    void focus_next_session() override {}
    void focus_prev_session() override {}

    auto focused_session() const -> std::shared_ptr<mir::scene::Session> override { return {}; }

    void set_focus_to(
        std::shared_ptr<mir::scene::Session> const& /*focus_session*/,
        std::shared_ptr<mir::scene::Surface> const& /*focus_surface*/) override {}

    auto focused_surface() const -> std::shared_ptr<mir::scene::Surface> override { return {}; }

    void raise(mir::shell::SurfaceSet const& /*windows*/) override {}

    virtual auto surface_at(mir::geometry::Point /*cursor*/) const -> std::shared_ptr<mir::scene::Surface> override
        { return {}; }

    void set_drag_and_drop_handle(std::vector<uint8_t> const& /*handle*/) override {}

    void clear_drag_and_drop_handle() override {}
};

struct StubDisplayLayout : mir::shell::DisplayLayout
{
    void clip_to_output(mir::geometry::Rectangle& /*rect*/) override {}

    void size_to_output(mir::geometry::Rectangle& /*rect*/) override {}

    bool place_in_output(mir::graphics::DisplayConfigurationOutputId /*id*/, mir::geometry::Rectangle& /*rect*/) override
        { return false; }
};

struct StubPersistentSurfaceStore : mir::shell::PersistentSurfaceStore
{
    Id id_for_surface(std::shared_ptr<mir::scene::Surface> const& /*surface*/) override { return {}; }

    auto surface_for_id(Id const& /*id*/) const -> std::shared_ptr<mir::scene::Surface> override { return {}; }
};

struct StubSurface : mir::test::doubles::StubSurface
{
    StubSurface(
        std::string name,
        MirWindowType type,
        mir::geometry::Point top_left,
        mir::geometry::Size size,
        MirDepthLayer depth_layer)
        : name_{name}, type_{type}, top_left_{top_left}, size_{size}, depth_layer_{depth_layer}
    {
    }

    std::string name() const override { return name_; };
    MirWindowType type() const override { return type_; }

    mir::geometry::Point top_left() const override { return top_left_; }
    void move_to(mir::geometry::Point const& top_left) override { top_left_ = top_left; }

    mir::geometry::Size size() const override { return  size_; }
    void resize(mir::geometry::Size const& size) override { size_ = size; }

    auto state() const -> MirWindowState override { return state_; }
    auto configure(MirWindowAttrib attrib, int value) -> int override {
        switch (attrib)
        {
        case mir_window_attrib_state:
            state_ = MirWindowState(value);
            return state_;
        default:
            return value;
        }
    }

    bool visible() const override { return  state() != mir_window_state_hidden; }

    auto depth_layer() const -> MirDepthLayer override { return depth_layer_; }
    void set_depth_layer(MirDepthLayer depth_layer) override { depth_layer_ = depth_layer; }

    std::string name_;
    MirWindowType type_;
    mir::geometry::Point top_left_;
    mir::geometry::Size size_;
    MirWindowState state_ = mir_window_state_restored;
    MirDepthLayer depth_layer_;
};

struct StubStubSession : mir::test::doubles::StubSession
{
    mir::frontend::SurfaceId create_surface(
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& /*sink*/) override
    {
        auto id = mir::frontend::SurfaceId{next_surface_id.fetch_add(1)};
        auto surface = std::make_shared<StubSurface>(
            params.name,
            params.type.value(),
            params.top_left,
            params.size,
            params.depth_layer.is_set() ?
                params.depth_layer.value()
                : mir_depth_layer_application);
        surfaces[id] = surface;
        return id;
    }

    std::shared_ptr<mir::scene::Surface> surface(mir::frontend::SurfaceId surface) const override
    {
        return surfaces.at(surface);
    }

private:
    std::atomic<int> next_surface_id;
    std::map<mir::frontend::SurfaceId, std::shared_ptr<mir::scene::Surface>> surfaces;
};

struct MockWindowManagerPolicy : miral::CanonicalWindowManagerPolicy
{
    using miral::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy;

    bool handle_touch_event(MirTouchEvent const* /*event*/) { return false; }
    bool handle_pointer_event(MirPointerEvent const* /*event*/) { return false; }
    bool handle_keyboard_event(MirKeyboardEvent const* /*event*/) { return false; }

    MOCK_METHOD1(advise_new_window, void (miral::WindowInfo const& window_info));
    MOCK_METHOD2(advise_move_to, void(miral::WindowInfo const& window_info, mir::geometry::Point top_left));
    MOCK_METHOD2(advise_resize, void(miral::WindowInfo const& window_info, mir::geometry::Size const& new_size));
    MOCK_METHOD1(advise_raise, void(std::vector<miral::Window> const&));
    MOCK_METHOD1(advise_output_create, void(miral::Output const&));
    MOCK_METHOD2(advise_output_update, void(miral::Output const&, miral::Output const&));
    MOCK_METHOD1(advise_output_delete, void(miral::Output const&));

    void handle_request_drag_and_drop(miral::WindowInfo& /*window_info*/) {}
    void handle_request_move(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/) {}
    void handle_request_resize(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/) {}
    mir::geometry::Rectangle confirm_placement_on_display(const miral::WindowInfo&, MirWindowState, mir::geometry::Rectangle const& new_placement)
    {
        return new_placement;
    }
};

struct TestDisplayConfigurationObserver : mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>
{
    void register_interest(std::weak_ptr<mir::graphics::DisplayConfigurationObserver> const& o) override
    {
        ASSERT_THAT(observer.lock(), testing::IsNull())
            << "TestDisplayConfigurationObserver does not support multiple observers";
        observer = o;
    }

    void register_interest(std::weak_ptr<mir::graphics::DisplayConfigurationObserver> const& o, mir::Executor&) override
    {
        register_interest(o);
    }

    void unregister_interest(mir::graphics::DisplayConfigurationObserver const& o) override
    {
        ASSERT_THAT(observer.lock().get(), testing::Eq(&o));
        observer = std::weak_ptr<mir::graphics::DisplayConfigurationObserver>(); // set to null
    }

    std::weak_ptr<mir::graphics::DisplayConfigurationObserver> observer;
};

struct TestWindowManagerTools : testing::Test
{
    StubFocusController focus_controller;
    StubDisplayLayout display_layout;
    StubPersistentSurfaceStore persistent_surface_store;
    TestDisplayConfigurationObserver display_configuration_observer;
    std::shared_ptr<StubStubSession> session{std::make_shared<StubStubSession>()};

    MockWindowManagerPolicy* window_manager_policy{nullptr};
    miral::WindowManagerTools window_manager_tools{nullptr};

    miral::BasicWindowManager basic_window_manager{
        &focus_controller,
        mir::test::fake_shared(display_layout),
        mir::test::fake_shared(persistent_surface_store),
        display_configuration_observer,
        [this](miral::WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
            {
                auto policy = std::make_unique<testing::NiceMock<MockWindowManagerPolicy>>(tools);
                window_manager_policy = policy.get();
                window_manager_tools = tools;
                return policy;
            }
    };

    static auto create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params) -> mir::frontend::SurfaceId
    {
        // This type is Mir-internal, I hope we don't need to create it here
        std::shared_ptr<mir::frontend::EventSink> const sink;
        return session->create_surface(params, sink);
    }

    void set_outputs(std::vector<miral::Rectangle> outputs)
    {
        auto const display_config = std::make_shared<const mir::test::doubles::MockDisplayConfiguration>();
        EXPECT_CALL(*display_config, for_each_output(testing::_))
            .WillOnce(testing::Invoke([outputs](std::function<void(mir::graphics::DisplayConfigurationOutput const&)> func){
                for (auto i = 0u; i < outputs.size(); i++)
                {
                    auto const& rect = outputs[i];
                    mir::graphics::DisplayConfigurationOutput display_config_output{
                        mir::graphics::DisplayConfigurationOutputId{(int)i}, // id
                        mir::graphics::DisplayConfigurationCardId{1}, // card_id
                        mir::graphics::DisplayConfigurationOutputType::unknown, // type
                        {mir_pixel_format_abgr_8888}, // pixel_formats
                        {{rect.size, 60}}, // modes
                        0, // prefered_mode_index
                        {rect.size}, // physical_size_mm
                        true, // connected
                        true, // used
                        rect.top_left, // top_left
                        0, // current_mode_index
                        mir_pixel_format_abgr_8888, // current_format
                        mir_power_mode_on, // power_mode
                        mir_orientation_normal, // orientation
                        1.0, // scale
                        mir_form_factor_unknown, // form_factor
                        mir_subpixel_arrangement_unknown, // subpixel_arrangement
                        {}, // gamma
                        mir_output_gamma_unsupported, // gamma_supported
                        {}, // edid
                        {}, // custom_logical_size
                    };
                    func(display_config_output);
                }
            }));
        auto config_observer = display_configuration_observer.observer.lock();
        ASSERT_THAT(config_observer, testing::NotNull());
        config_observer->configuration_applied(display_config);
    }
};

#endif //MIRAL_TEST_WINDOW_MANAGER_TOOLS_H
