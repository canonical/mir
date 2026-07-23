/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_method_v1.h"

#include "input_method_common.h"
#include "output_manager.h"
#include "wayland_rs/src/ffi.rs.h"
#include "window_wl_surface_role.h"
#include "wl_surface.h"

#include <mir/executor.h>
#include <mir/log.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/scene/text_input_hub.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

#include <deque>
#include <ranges>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

namespace
{
class InputMethodV1 : public mw::InputMethodV1
{
public:
    InputMethodV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::InputMethodV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<ms::TextInputHub> text_input_hub)
        : mw::InputMethodV1{std::move(client), std::move(instance), object_id},
          text_input_hub{std::move(text_input_hub)},
          state_observer{std::make_shared<StateObserver>(this)}
    {
        this->text_input_hub->register_interest(state_observer, *wayland_executor);
    }

    ~InputMethodV1() override
    {
        text_input_hub->unregister_interest(*state_observer);
    }

    void activated(
        ms::TextInputStateSerial serial,
        bool new_input_field,
        ms::TextInputState const& state)
    {
        if (!is_activated || new_input_field)
        {
            deactivated();

            std::shared_ptr<InputMethodContextV1> created_context;
            auto const context_base = mw::create_zwp_input_method_context_v1(
                *client->raw_client(),
                get_box()->version(),
                [&](rust::Box<mw::InputMethodContextV1Middleware> instance, uint32_t object_id)
                    -> std::shared_ptr<mw::InputMethodContextV1>
                {
                    created_context = std::make_shared<InputMethodContextV1>(
                        client,
                        std::move(instance),
                        object_id,
                        text_input_hub);
                    return created_context;
                });
            if (!created_context)
            {
                throw std::logic_error{"failed to create zwp_input_method_context_v1"};
            }
            (void)context_base;
            context = created_context;
            is_activated = true;
            cached_state = ms::TextInputState{};
            send_activate_event(context->get_box());
            context->send_reset_event();
        }

        if (cached_state.change_cause != state.change_cause)
        {
            cached_state.change_cause = state.change_cause;
            if (cached_state.change_cause.has_value())
            {
                switch (cached_state.change_cause.value())
                {
                    case ms::TextInputChangeCause::other:
                        context->reset_pending_change();
                        context->send_reset_event();
                        break;
                    default:
                        break;
                }
            }
        }

        if (cached_state.surrounding_text != state.surrounding_text ||
            cached_state.cursor != state.cursor ||
            cached_state.anchor != state.anchor)
        {
            cached_state.surrounding_text = state.surrounding_text;
            cached_state.cursor = state.cursor;
            cached_state.anchor = state.anchor;
            context->send_surrounding_text_event(
                state.surrounding_text.value_or(""),
                state.cursor.value_or(0),
                state.anchor.value_or(0));
        }

        if (cached_state.content_hint != state.content_hint || cached_state.content_purpose != state.content_purpose)
        {
            cached_state.content_hint = state.content_hint;
            cached_state.content_purpose = state.content_purpose;
            context->send_content_type_event(
                mf::mir_to_wayland_content_hint(state.content_hint.value_or(ms::TextInputContentHint::none)),
                mf::mir_to_wayland_content_purpose(state.content_purpose.value_or(ms::TextInputContentPurpose::normal)));
        }

        context->add_serial(serial);
    }

    void deactivated()
    {
        if (is_activated)
        {
            is_activated = false;
            if (context)
            {
                context_on_deathbed = context;
                send_deactivate_event(context);
                context = nullptr;
            }
        }
    }

private:
    struct StateObserver : ms::TextInputStateObserver
    {
        explicit StateObserver(InputMethodV1* input_method)
            : input_method{input_method}
        {
        }

        void activated(
            ms::TextInputStateSerial serial,
            bool new_input_field,
            ms::TextInputState const& state) override
        {
            if (input_method)
                input_method.value().activated(serial, new_input_field, state);
        }

        void deactivated() override
        {
            if (input_method)
                input_method.value().deactivated();
        }

        void show_input_panel() override {}
        void hide_input_panel() override {}

        mw::Weak<InputMethodV1> const input_method;
    };

    class InputMethodContextV1 : public mw::InputMethodContextV1
    {
    public:
        InputMethodContextV1(
            std::shared_ptr<mw::Client> client,
            rust::Box<mw::InputMethodContextV1Middleware> instance,
            uint32_t object_id,
            std::shared_ptr<ms::TextInputHub> text_input_hub)
            : mw::InputMethodContextV1{std::move(client), std::move(instance), object_id},
              text_input_hub{std::move(text_input_hub)}
        {
        }

        void add_serial(ms::TextInputStateSerial serial)
        {
            send_commit_state_event(done_event_count);
            serials.push_back({done_event_count, serial});
            while (serials.size() > max_remembered_serials)
            {
                serials.pop_front();
            }
            done_event_count++;
        }

        void reset_pending_change()
        {
            change = InputMethodV1Change();
        }

    private:
        enum class InputMethodV1ChangeWaitingStatus
        {
            none,
            commit_string,
            preedit_string,
        };

        struct InputMethodV1Change
        {
            ms::TextInputChange pending_change{{}};
            InputMethodV1ChangeWaitingStatus waiting_status;

            void reset()
            {
                pending_change = ms::TextInputChange{{}};
                waiting_status = InputMethodV1ChangeWaitingStatus::none;
            }

            void check_waiting_status(InputMethodV1ChangeWaitingStatus expected)
            {
                if (waiting_status != InputMethodV1ChangeWaitingStatus::none && expected != waiting_status)
                {
                    reset();
                }
            }
        };

        auto find_serial(uint32_t done_count) const -> std::optional<ms::TextInputStateSerial>
        {
            for (auto const& serial_pair : std::ranges::reverse_view(serials))
            {
                if (serial_pair.first == done_count)
                {
                    return serial_pair.second;
                }
            }
            return std::nullopt;
        }

        void on_text_changed(uint32_t serial)
        {
            auto const mir_serial = find_serial(serial);
            if (mir_serial)
            {
                change.pending_change.serial = *mir_serial;
                text_input_hub->text_changed(change.pending_change);
            }
            else
            {
                mir::log_warning("%s: invalid commit serial %d", "zwp_input_method_context_v1", serial);
            }

            change.reset();
        }

        auto commit_string(uint32_t serial, rust::String text) -> void override
        {
            change.check_waiting_status(InputMethodV1ChangeWaitingStatus::commit_string);
            change.pending_change.commit_text = std::string{text};
            on_text_changed(serial);
        }

        auto preedit_string(uint32_t serial, rust::String text, rust::String commit) -> void override
        {
            change.check_waiting_status(InputMethodV1ChangeWaitingStatus::preedit_string);
            change.pending_change.preedit_text = std::string{text};
            change.pending_change.preedit_commit = std::string{commit};
            on_text_changed(serial);
        }

        auto preedit_styling(uint32_t index, uint32_t length, uint32_t style) -> void override
        {
            change.pending_change.preedit_style = { index, length, style };
            change.waiting_status = InputMethodV1ChangeWaitingStatus::preedit_string;
        }

        auto preedit_cursor(int32_t index) -> void override
        {
            change.pending_change.preedit_cursor_begin = index;
            change.pending_change.preedit_cursor_end = index;
            change.waiting_status = InputMethodV1ChangeWaitingStatus::preedit_string;
        }

        auto delete_surrounding_text(int32_t index, uint32_t length) -> void override
        {
            change.pending_change.cursor_position = { index, 0 };
            change.pending_change.delete_after = length;
            change.pending_change.delete_before = 0;
            change.waiting_status = InputMethodV1ChangeWaitingStatus::commit_string;
        }

        auto cursor_position(int32_t index, int32_t anchor) -> void override
        {
            change.pending_change.cursor_position = { index, anchor };
            change.waiting_status = InputMethodV1ChangeWaitingStatus::commit_string;
        }

        auto modifiers_map(rust::Vec<uint8_t> map) -> void override
        {
            change.pending_change.modifier_map = std::vector<uint8_t>(map.begin(), map.end());
            change.waiting_status = InputMethodV1ChangeWaitingStatus::none;
        }

        auto keysym(uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers) -> void override
        {
            change.pending_change.keysym = { time, sym, state, modifiers };
            on_text_changed(serial);
        }

        auto grab_keyboard(rust::Box<mw::KeyboardMiddleware>, uint32_t) -> std::shared_ptr<mw::Keyboard> override
        {
            return nullptr;
        }

        auto key(uint32_t, uint32_t, uint32_t, uint32_t) -> void override
        {
        }

        auto modifiers(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) -> void override
        {
        }

        auto language(uint32_t, rust::String) -> void override
        {
        }

        auto text_direction(uint32_t serial, uint32_t direction) -> void override
        {
            change.pending_change.direction = direction;
            on_text_changed(serial);
        }

        std::shared_ptr<ms::TextInputHub> const text_input_hub;
        InputMethodV1Change change;
        static size_t constexpr max_remembered_serials{10};
        std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> serials;
        uint32_t done_event_count{0};
    };

    std::shared_ptr<ms::TextInputHub> const text_input_hub;
    std::shared_ptr<StateObserver> const state_observer;
    bool is_activated = false;
    std::shared_ptr<InputMethodContextV1> context = nullptr;
    std::shared_ptr<InputMethodContextV1> context_on_deathbed = nullptr;
    ms::TextInputState cached_state{};
};

class InputPanelV1 : public mw::InputPanelV1
{
public:
    InputPanelV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::InputPanelV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<mir::shell::Shell> shell,
        mf::WlSeat* seat,
        mf::OutputManager* output_manager,
        std::shared_ptr<ms::TextInputHub> text_input_hub,
        std::shared_ptr<mf::SurfaceRegistry> surface_registry)
        : mw::InputPanelV1{std::move(client), std::move(instance), object_id},
          wayland_executor{std::move(wayland_executor)},
          shell{std::move(shell)},
          seat{seat},
          output_manager{output_manager},
          text_input_hub{std::move(text_input_hub)},
          surface_registry{std::move(surface_registry)}
    {
    }

private:
    class InputPanelSurfaceV1
        : public mw::InputPanelSurfaceV1,
          public mf::WindowWlSurfaceRole
    {
    public:
        InputPanelSurfaceV1(
            std::shared_ptr<mw::Client> client,
            rust::Box<mw::InputPanelSurfaceV1Middleware> instance,
            uint32_t object_id,
            std::shared_ptr<mir::Executor> const& wayland_executor,
            mf::WlSeat* seat,
            mf::WlSurface* surface,
            std::shared_ptr<mir::shell::Shell> const& shell,
            mf::OutputManager* output_manager,
            std::shared_ptr<ms::TextInputHub> text_input_hub,
            std::shared_ptr<mf::SurfaceRegistry> const& surface_registry)
            : mw::InputPanelSurfaceV1{client, std::move(instance), object_id},
              mf::WindowWlSurfaceRole(
                  *wayland_executor,
                  seat,
                  client,
                  surface,
                  shell,
                  output_manager,
                  surface_registry),
              text_input_hub{std::move(text_input_hub)},
              state_observer{std::make_shared<StateObserver>(this)}
        {
            this->text_input_hub->register_interest(state_observer, *wayland_executor);
            mir::shell::SurfaceSpecification spec;
            spec.state = mir_window_state_hidden;
            spec.type = MirWindowType::mir_window_type_inputmethod;
            spec.depth_layer = MirDepthLayer::mir_depth_layer_always_on_top;
            apply_spec(spec);
        }

        ~InputPanelSurfaceV1() override
        {
            text_input_hub->unregister_interest(*state_observer);
        }

        auto destroyed_flag() const -> std::shared_ptr<bool const> { return mw::InputPanelSurfaceV1::destroyed_flag(); }

        void show()
        {
            remove_state_now(mir_window_state_hidden);
            add_state_now(mir_window_state_attached);
        }

        void hide()
        {
            add_state_now(mir_window_state_hidden);
            remove_state_now(mir_window_state_attached);
        }

        void handle_state_change(MirWindowState) override {}
        void handle_active_change(bool) override {}
        void handle_resize(std::optional<mir::geometry::Point> const&, mir::geometry::Size const&) override {}
        void handle_close_request() override {}
        void handle_tiled_edges(mir::Flags<MirTiledEdge>) override {}

        void handle_commit() override
        {
            auto surface_opt = scene_surface();
            if (!surface_opt)
                return;

            auto surface = surface_opt->get();
            auto input_region_list = surface->get_input_region();
            auto next_bounds = input_region_list.empty() ? surface->input_bounds() : input_region_list[0];
            if (last_bounds == next_bounds)
                return;

            mir::shell::SurfaceSpecification spec;
            spec.exclusive_rect = std::optional<mir::geometry::Rectangle>(next_bounds);
            last_bounds = next_bounds;
            apply_spec(spec);
        }

        void destroy_role() const override
        {
            const_cast<InputPanelSurfaceV1*>(this)->destroy_and_delete();
        }

    private:
        struct StateObserver : ms::TextInputStateObserver
        {
            explicit StateObserver(InputPanelSurfaceV1* input_panel_surface)
                : input_panel_surface{input_panel_surface}
            {
            }

            void activated(ms::TextInputStateSerial, bool, ms::TextInputState const&) override {}
            void deactivated() override {}

            void show_input_panel() override
            {
                input_panel_surface.value().show();
            }

            void hide_input_panel() override
            {
                input_panel_surface.value().hide();
            }

            mw::Weak<InputPanelSurfaceV1> const input_panel_surface;
        };

        using mw::InputPanelSurfaceV1::set_toplevel;

        auto set_toplevel(mw::Weak<mw::Output> const& output, uint32_t position) -> void override
        {
            mir::shell::SurfaceSpecification spec;
            auto const output_id = mf::OutputManager::output_id_for(mw::as_nullable_ptr(output));
            if (output_id)
            {
                spec.output_id = output_id.value();
            }
            if (position == Position::center_bottom)
            {
                spec.attached_edges = MirPlacementGravity::mir_placement_gravity_south;
            }
            else
            {
                mir::log_warning("Invalid position: %u", position);
            }

            apply_spec(spec);
            show();
        }

        auto set_overlay_panel() -> void override
        {
        }

        std::shared_ptr<ms::TextInputHub> const text_input_hub;
        std::shared_ptr<StateObserver> const state_observer;
        mir::geometry::Rectangle last_bounds;
    };

    using mw::InputPanelV1::get_input_panel_surface;

    auto get_input_panel_surface(
        mw::Weak<mw::Surface> const& surface,
        rust::Box<mw::InputPanelSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::InputPanelSurfaceV1> override
    {
        return std::make_shared<InputPanelSurfaceV1>(
            client,
            std::move(child_instance),
            child_object_id,
            wayland_executor,
            seat,
            mw::Surface::from<mf::WlSurface>(surface),
            shell,
            output_manager,
            text_input_hub,
            surface_registry);
    }

    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::shell::Shell> const shell;
    mf::WlSeat* seat;
    mf::OutputManager* const output_manager;
    std::shared_ptr<ms::TextInputHub> const text_input_hub;
    std::shared_ptr<mf::SurfaceRegistry> const surface_registry;
};
}

auto mf::create_zwp_input_method_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::InputMethodV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::TextInputHub> text_input_hub)
-> std::shared_ptr<mw::InputMethodV1>
{
    return std::make_shared<InputMethodV1>(
        std::move(client),
        std::move(instance),
        object_id,
        std::move(wayland_executor),
        std::move(text_input_hub));
}

auto mf::create_zwp_input_panel_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::InputPanelV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<shell::Shell> shell,
    WlSeat* seat,
    OutputManager* output_manager,
    std::shared_ptr<scene::TextInputHub> text_input_hub,
    std::shared_ptr<SurfaceRegistry> surface_registry)
-> std::shared_ptr<mw::InputPanelV1>
{
    return std::make_shared<InputPanelV1>(
        std::move(client),
        std::move(instance),
        object_id,
        std::move(wayland_executor),
        std::move(shell),
        seat,
        output_manager,
        std::move(text_input_hub),
        std::move(surface_registry));
}
