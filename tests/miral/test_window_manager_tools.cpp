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

#include "test_window_manager_tools.h"

#include "basic_window_manager.h"

#include <miral/canonical_window_manager.h>
#include <miral/output.h>

#include <mir/shell/display_layout.h>
#include <mir/shell/focus_controller.h>
#include <mir/shell/persistent_surface_store.h>
#include "mir/graphics/display_configuration_observer.h"
#include <mir/wayland/weak.h>

#include <mir/test/doubles/stub_session.h>
#include <mir/test/doubles/stub_surface.h>
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/stub_input_device_registry.h>
#include <mir/test/doubles/stub_main_loop.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>

namespace
{

struct StubFocusController : mir::shell::FocusController
{
    void focus_next_session() override {}
    void focus_prev_session() override {}

    auto focused_session() const -> std::shared_ptr<mir::scene::Session> override { return {}; }

    void set_popup_grab_tree(std::shared_ptr<mir::scene::Surface> const& /*surface*/) override {}

    void set_focus_to(
        std::shared_ptr<mir::scene::Session> const& /*focus_session*/,
        std::shared_ptr<mir::scene::Surface> const& /*focus_surface*/) override {}

    auto focused_surface() const -> std::shared_ptr<mir::scene::Surface> override { return {}; }

    void raise(mir::shell::SurfaceSet const& /*windows*/) override {}

    virtual auto surface_at(mir::geometry::Point /*cursor*/) const -> std::shared_ptr<mir::scene::Surface> override
        { return {}; }

    void swap_z_order(mir::shell::SurfaceSet const& /*first*/, mir::shell::SurfaceSet const& /*second*/) override {}

    void send_to_back(mir::shell::SurfaceSet const& /*windows*/) override {}

    auto is_above(std::weak_ptr<mir::scene::Surface> const& /*a*/, std::weak_ptr<mir::scene::Surface> const& /*b*/) const -> bool override
    {
        return false;
    }
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
        MirDepthLayer depth_layer,
        MirFocusMode focus_mode)
        : name_{name}, type_{type}, top_left_{top_left}, size_{size}, depth_layer_{depth_layer}, focus_mode_{focus_mode}
    {
    }

    std::string name() const override { return name_; };
    MirWindowType type() const override { return type_; }

    mir::geometry::Point top_left() const override { return top_left_; }
    void move_to(mir::geometry::Point const& top_left) override { top_left_ = top_left; }

    mir::geometry::Size window_size() const override { return  size_; }
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

    mir::geometry::Displacement content_offset() const override { return content_offset_; }
    mir::geometry::Size content_size() const override
    {
        return {size_.width - content_size_offset.dx, size_.height - content_size_offset.dy};
    }
    void set_window_margins(
        mir::geometry::DeltaY top,
        mir::geometry::DeltaX left,
        mir::geometry::DeltaY bottom,
        mir::geometry::DeltaX right) override
    {
        content_offset_ = mir::geometry::Displacement{left, top};
        content_size_offset = mir::geometry::Displacement{left + right, top + bottom};
    }

    auto focus_mode() const -> MirFocusMode override { return focus_mode_; }
    void set_focus_mode(MirFocusMode mode) override { focus_mode_ = mode; }

    std::string name_;
    MirWindowType type_;
    mir::geometry::Point top_left_;
    mir::geometry::Size size_;
    MirWindowState state_ = mir_window_state_restored;
    MirDepthLayer depth_layer_;
    mir::geometry::Displacement content_offset_;
    mir::geometry::Displacement content_size_offset;
    MirFocusMode focus_mode_;
};

struct MockWindowManagerPolicy : miral::CanonicalWindowManagerPolicy
{
    using miral::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy;

    bool handle_touch_event(MirTouchEvent const* /*event*/) { return false; }
    bool handle_pointer_event(MirPointerEvent const* /*event*/) { return false; }
    bool handle_keyboard_event(MirKeyboardEvent const* /*event*/) { return false; }

    MOCK_METHOD(void, advise_new_window, (miral::WindowInfo const& window_info), ());
    MOCK_METHOD(void, advise_move_to, (miral::WindowInfo const& window_info, mir::geometry::Point top_left), ());
    MOCK_METHOD(void, advise_resize, (miral::WindowInfo const& window_info, mir::geometry::Size const& new_size), ());
    MOCK_METHOD(void, advise_raise, (std::vector<miral::Window> const&), ());
    MOCK_METHOD(void, advise_output_create, (miral::Output const&), ());
    MOCK_METHOD(void, advise_output_update, (miral::Output const&, miral::Output const&), ());
    MOCK_METHOD(void, advise_output_delete, (miral::Output const&), ());

    void handle_request_move(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/) {}
    void handle_request_resize(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/) {}
    mir::geometry::Rectangle confirm_placement_on_display(const miral::WindowInfo&, MirWindowState, mir::geometry::Rectangle const& new_placement)
    {
        return new_placement;
    }
};

struct FakeDisplayConfigurationObserverRegistrar : mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>
{
    void register_interest(std::weak_ptr<mir::graphics::DisplayConfigurationObserver> const& o) override
    {
        ASSERT_THAT(observer.lock(), testing::IsNull())
            << "FakeDisplayConfigurationObserverRegistrar does not support multiple observers";
        observer = o;
    }

    void register_interest(std::weak_ptr<mir::graphics::DisplayConfigurationObserver> const& o, mir::Executor&) override
    {
        register_interest(o);
    }

    void register_early_observer(
        std::weak_ptr<mir::graphics::DisplayConfigurationObserver> const& o,
        mir::Executor&) override
    {
        register_interest(o);
    }

    void unregister_interest(mir::graphics::DisplayConfigurationObserver const& o) override
    {
        ASSERT_THAT(observer.lock().get(), testing::Eq(&o));
        observer = std::weak_ptr<mir::graphics::DisplayConfigurationObserver>(); // set to null
    }

    void notify_configuration_applied(
        std::shared_ptr<mir::graphics::DisplayConfiguration const> display_config)
    {
        auto config_observer = observer.lock();
        EXPECT_THAT(config_observer, testing::NotNull());
        config_observer->configuration_applied(display_config);
    }

    std::weak_ptr<mir::graphics::DisplayConfigurationObserver> observer;
};

struct FakeDisplayConfiguration : public mir::graphics::DisplayConfiguration
{
    FakeDisplayConfiguration(std::vector<mir::graphics::DisplayConfigurationOutput> const& outputs)
        : outputs{outputs}
    {
    }

    void for_each_output(std::function<void(mir::graphics::DisplayConfigurationOutput const&)> f) const override
    {
        for (auto const& output : outputs)
        {
            f(output);
        }
    }

    void for_each_output(std::function<void(mir::graphics::UserDisplayConfigurationOutput&)> /*f*/) override
    {
        throw std::runtime_error(
            "FakeDisplayConfiguration::for_each_output(function<void(UserDisplayConfigurationOutput&)>)"
            " is not implemented");
    }

    virtual std::unique_ptr<DisplayConfiguration> clone() const override
    {
        throw std::runtime_error("FakeDisplayConfiguration::clone is not implemented");
    }

    std::vector<mir::graphics::DisplayConfigurationOutput> const outputs;
};

}

namespace mt = mir::test;

struct mt::TestWindowManagerTools::Self
{
    StubFocusController focus_controller;
    StubDisplayLayout display_layout;
    StubPersistentSurfaceStore persistent_surface_store;
    FakeDisplayConfigurationObserverRegistrar display_configuration_observer;
    mt::doubles::StubInputDeviceRegistry input_device_registry;
    mt::doubles::StubMainLoop main_loop;
};

mt::TestWindowManagerTools::TestWindowManagerTools()
    : self{std::make_unique<Self>()},
      session{std::make_shared<StubStubSession>()},
      window_manager_policy{nullptr},
      window_manager_tools{nullptr},
      basic_window_manager{
        &self->focus_controller,
        mir::test::fake_shared(self->display_layout),
        mir::test::fake_shared(self->persistent_surface_store),
        self->display_configuration_observer,
        mir::test::fake_shared(self->input_device_registry),
        [this](miral::WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
            {
                auto policy = std::make_unique<testing::NiceMock<MockWindowManagerPolicy>>(tools);
                window_manager_policy = policy.get();
                window_manager_tools = tools;
                return policy;
            }
    }
{
}

mt::TestWindowManagerTools::~TestWindowManagerTools() = default;

auto mt::TestWindowManagerTools::create_surface(
    std::shared_ptr<mir::scene::Session> const& session,
    mir::shell::SurfaceSpecification const& params) -> std::shared_ptr<mir::scene::Surface>
{
    // This type is Mir-internal, I hope we don't need to create it here
    std::shared_ptr<mir::scene::SurfaceObserver> const observer;
    return session->create_surface(nullptr, {}, params, observer, nullptr);
}

auto mt::TestWindowManagerTools::create_fake_display_configuration(std::vector<miral::Rectangle> const& outputs)
    -> std::shared_ptr<graphics::DisplayConfiguration const>
{
    std::vector<std::pair<graphics::DisplayConfigurationLogicalGroupId, miral::Rectangle>> outputs_with_ids;
    for (auto const& output : outputs)
    {
        outputs_with_ids.push_back(std::make_pair(graphics::DisplayConfigurationLogicalGroupId{0}, output));
    }
    return create_fake_display_configuration(outputs_with_ids);
}

auto mt::TestWindowManagerTools::create_fake_display_configuration(
    std::vector<std::pair<graphics::DisplayConfigurationLogicalGroupId, miral::Rectangle>> const& outputs)
    -> std::shared_ptr<graphics::DisplayConfiguration const>
{
    std::vector<mir::graphics::DisplayConfigurationOutput> config_outputs;
    for (auto i = 0u; i < outputs.size(); i++)
    {
        auto const& rect = outputs[i].second;
        config_outputs.push_back(mir::graphics::DisplayConfigurationOutput{
            mir::graphics::DisplayConfigurationOutputId{static_cast<int>(i + 1)}, // id
            mir::graphics::DisplayConfigurationCardId{1}, // card_id
            outputs[i].first, // logical_group_id
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
        });
    }
    return std::make_shared<FakeDisplayConfiguration const>(config_outputs);
}

void mt::TestWindowManagerTools::notify_configuration_applied(
    std::shared_ptr<graphics::DisplayConfiguration const> display_config)
{
    self->display_configuration_observer.notify_configuration_applied(display_config);
}

auto mt::TestWindowManagerTools::create_and_select_window(
    mir::shell::SurfaceSpecification& creation_parameters) -> miral::Window
{
    return create_and_select_window_for_session(creation_parameters, std::make_shared<StubStubSession>());
}

auto mt::TestWindowManagerTools::create_and_select_window_for_session(
    mir::shell::SurfaceSpecification& creation_parameters,
    std::shared_ptr<scene::Session> session_to_add) -> miral::Window
{
    miral::Window result;

    EXPECT_CALL(*window_manager_policy, advise_new_window(testing::_))
        .WillOnce(
            testing::Invoke(
                [&result](miral::WindowInfo const& window_info)
                { result = window_info.window(); }));

    basic_window_manager.add_session(session_to_add);
    basic_window_manager.add_surface(session_to_add, creation_parameters, &create_surface);
    basic_window_manager.select_active_window(result);

    // Clear the expectations used to capture parent & child
    testing::Mock::VerifyAndClearExpectations(window_manager_policy);

    return result;
}

auto mt::StubStubSession::create_surface(
    std::shared_ptr<mir::scene::Session> const& /*session*/,
    mir::wayland::Weak<mir::frontend::WlSurface> const& /*wayland_surface*/,
    mir::shell::SurfaceSpecification const& params,
    std::shared_ptr<mir::scene::SurfaceObserver> const& /*observer*/,
    mir::Executor*) -> std::shared_ptr<mir::scene::Surface>
{
    auto id = mir::frontend::SurfaceId{next_surface_id.fetch_add(1)};
    auto surface = std::make_shared<StubSurface>(
        params.name.is_set() ? params.name.value() : "",
        params.type.is_set() ?
        params.type.value()
                             : mir_window_type_normal,
        params.top_left.is_set() ?
        params.top_left.value()
                                 : mir::geometry::Point{},
        mir::geometry::Size{
            params.width.is_set() ? params.width.value() : mir::geometry::Width{100},
            params.height.is_set() ? params.height.value() : mir::geometry::Height{100},
        },
        params.depth_layer.is_set() ?
        params.depth_layer.value()
                                    : mir_depth_layer_application,
        params.focus_mode.is_set() ?
        params.focus_mode.value()
                                   : mir_focus_mode_focusable);
    surfaces[id] = surface;
    return surface;
}
