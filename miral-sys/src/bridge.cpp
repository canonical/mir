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

#include "bridge.h"
#include "miral-sys/src/lib.rs.h"

#include <miral/window_management_policy.h>
#include <miral/window_specification.h>
#include <miral/output.h>
#include <miral/zone.h>
#include <miral/set_window_management_policy.h>
#include <miral/decorations.h>
#include <miral/keymap.h>
#include <miral/x11_support.h>
#include <miral/configuration_option.h>

#include <mir_toolkit/events/enums.h>
#include <mir_toolkit/events/input/keyboard_event.h>
#include <mir_toolkit/events/input/pointer_event.h>
#include <mir_toolkit/events/input/touch_event.h>
#include <mir_toolkit/events/input/input_event.h>

#include <vector>
#include <string>
#include <functional>
#include <atomic>

// Forward declaration — we only need the pointer value, not the full type
namespace mir { namespace scene { class Surface; } }


namespace miral_sys
{

// Global atomic pointer to the active ExternalClientLauncher (set during server run).
// Atomic so it can be safely set from the start callback thread and read from
// the input/policy thread without a mutex.
namespace
{
    std::atomic<miral::ExternalClientLauncher*> g_active_launcher{nullptr};
}

// --- Helper conversions ---

static Point to_point(mir::geometry::Point p)
{
    return Point{p.x.as_int(), p.y.as_int()};
}

static Size to_size(mir::geometry::Size s)
{
    return Size{s.width.as_int(), s.height.as_int()};
}

static Rectangle to_rectangle(mir::geometry::Rectangle r)
{
    return Rectangle{to_point(r.top_left), to_size(r.size)};
}

static mir::geometry::Point from_point(Point p)
{
    return mir::geometry::Point{p.x, p.y};
}

static mir::geometry::Size from_size(Size s)
{
    return mir::geometry::Size{s.width, s.height};
}

static mir::geometry::Displacement from_displacement(Displacement d)
{
    return mir::geometry::Displacement{d.dx, d.dy};
}

static uint64_t window_to_id(miral::Window const& window)
{
    auto surface = static_cast<std::shared_ptr<mir::scene::Surface>>(window);
    return reinterpret_cast<uint64_t>(surface.get());
}

// --- MiralTools window registry ---

void MiralTools::register_window(miral::Window const& window)
{
    auto id = window_to_id(window);
    if (id != 0)
        window_registry[id] = window;
}

void MiralTools::unregister_window(uint64_t id)
{
    window_registry.erase(id);
}

miral::Window const* MiralTools::lookup_window(uint64_t id) const
{
    auto it = window_registry.find(id);
    if (it != window_registry.end())
        return &it->second;
    return nullptr;
}

// --- RustWindowManagementPolicy ---
// C++ subclass that dispatches all virtual methods to Rust

class RustWindowManagementPolicy : public miral::WindowManagementPolicy
{
public:
    RustWindowManagementPolicy(
        miral::WindowManagerTools const& tools,
        rust::Box<RustPolicyHolder> holder)
        : tools_{std::make_unique<MiralTools>(tools)},
          holder_{std::move(holder)}
    {
    }

    // Access the tools (for registering windows during dispatch)
    MiralTools& miral_tools() { return *tools_; }

    // Access the holder (needed during construction to set tools pointer)
    RustPolicyHolder& holder() { return *holder_; }

    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override
    {
        MiralApplicationInfo app_wrap{app_info};

        // Convert requested spec to bridge format
        auto has_parent = requested_specification.parent().is_set();
        uint64_t parent_id = 0;
        if (has_parent)
        {
            if (auto parent_surface = requested_specification.parent().value().lock())
                parent_id = reinterpret_cast<uint64_t>(parent_surface.get());
        }

        WindowSpecData spec_data{
            requested_specification.top_left().is_set(),
            requested_specification.top_left().is_set()
                ? to_point(requested_specification.top_left().value())
                : Point{0, 0},
            requested_specification.size().is_set(),
            requested_specification.size().is_set()
                ? to_size(requested_specification.size().value())
                : Size{0, 0},
            requested_specification.state().is_set(),
            requested_specification.state().is_set()
                ? static_cast<int32_t>(requested_specification.state().value())
                : 0,
            requested_specification.type().is_set(),
            requested_specification.type().is_set()
                ? static_cast<int32_t>(requested_specification.type().value())
                : 0,
            requested_specification.name().is_set(),
            requested_specification.name().is_set()
                ? rust::String(requested_specification.name().value())
                : rust::String(""),
            has_parent,
            parent_id,
            requested_specification.min_width().is_set(),
            requested_specification.min_width().is_set()
                ? requested_specification.min_width().value().as_int()
                : 0,
            requested_specification.min_height().is_set(),
            requested_specification.min_height().is_set()
                ? requested_specification.min_height().value().as_int()
                : 0,
            requested_specification.max_width().is_set(),
            requested_specification.max_width().is_set()
                ? requested_specification.max_width().value().as_int()
                : 0,
            requested_specification.max_height().is_set(),
            requested_specification.max_height().is_set()
                ? requested_specification.max_height().value().as_int()
                : 0,
            requested_specification.depth_layer().is_set(),
            requested_specification.depth_layer().is_set()
                ? static_cast<int32_t>(requested_specification.depth_layer().value())
                : 0,
            requested_specification.focus_mode().is_set(),
            requested_specification.focus_mode().is_set()
                ? static_cast<int32_t>(requested_specification.focus_mode().value())
                : 0,
            requested_specification.shell_chrome().is_set(),
            requested_specification.shell_chrome().is_set()
                ? static_cast<int32_t>(requested_specification.shell_chrome().value())
                : 0,
            requested_specification.preferred_orientation().is_set(),
            requested_specification.preferred_orientation().is_set()
                ? static_cast<int32_t>(requested_specification.preferred_orientation().value())
                : 0,
            requested_specification.confine_pointer().is_set(),
            requested_specification.confine_pointer().is_set()
                ? static_cast<int32_t>(requested_specification.confine_pointer().value())
                : 0,
            requested_specification.attached_edges().is_set(),
            requested_specification.attached_edges().is_set()
                ? static_cast<uint32_t>(requested_specification.attached_edges().value())
                : 0u,
            requested_specification.visible_on_lock_screen().is_set(),
            requested_specification.visible_on_lock_screen().is_set()
                ? requested_specification.visible_on_lock_screen().value()
                : false,
        };

        auto result = rust_policy_place_new_window(*holder_, app_wrap, spec_data);

        // Convert result back to miral::WindowSpecification
        miral::WindowSpecification out{requested_specification};
        if (result.has_top_left)
            out.top_left() = from_point(result.top_left);
        if (result.has_size)
            out.size() = from_size(result.size);
        if (result.has_state)
            out.state() = static_cast<MirWindowState>(result.state);
        if (result.has_type)
            out.type() = static_cast<MirWindowType>(result.window_type);
        if (result.has_parent)
        {
            if (auto const* w = tools_->lookup_window(result.parent_id))
                out.parent() = static_cast<std::shared_ptr<mir::scene::Surface>>(*w);
        }
        if (result.has_min_width)
            out.min_width() = mir::geometry::Width{result.min_width};
        if (result.has_min_height)
            out.min_height() = mir::geometry::Height{result.min_height};
        if (result.has_max_width)
            out.max_width() = mir::geometry::Width{result.max_width};
        if (result.has_max_height)
            out.max_height() = mir::geometry::Height{result.max_height};
        if (result.has_depth_layer)
            out.depth_layer() = static_cast<MirDepthLayer>(result.depth_layer);
        if (result.has_focus_mode)
            out.focus_mode() = static_cast<MirFocusMode>(result.focus_mode);
        if (result.has_shell_chrome)
            out.shell_chrome() = static_cast<MirShellChrome>(result.shell_chrome);
        if (result.has_preferred_orientation)
            out.preferred_orientation() = static_cast<MirOrientationMode>(result.preferred_orientation);
        if (result.has_confine_pointer)
            out.confine_pointer() = static_cast<MirPointerConfinementState>(result.confine_pointer);
        if (result.has_attached_edges)
            out.attached_edges() = static_cast<MirPlacementGravity>(result.attached_edges);
        if (result.has_visible_on_lock_screen)
            out.visible_on_lock_screen() = result.visible_on_lock_screen;
        return out;
    }

    void handle_window_ready(miral::WindowInfo& window_info) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_handle_window_ready(*holder_, info_wrap);
    }

    void handle_modify_window(
        miral::WindowInfo& window_info,
        miral::WindowSpecification const& modifications) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};

        auto mod_has_parent = modifications.parent().is_set();
        uint64_t mod_parent_id = 0;
        if (mod_has_parent)
        {
            if (auto parent_surface = modifications.parent().value().lock())
                mod_parent_id = reinterpret_cast<uint64_t>(parent_surface.get());
        }

        WindowSpecData spec_data{
            modifications.top_left().is_set(),
            modifications.top_left().is_set()
                ? to_point(modifications.top_left().value())
                : Point{0, 0},
            modifications.size().is_set(),
            modifications.size().is_set()
                ? to_size(modifications.size().value())
                : Size{0, 0},
            modifications.state().is_set(),
            modifications.state().is_set()
                ? static_cast<int32_t>(modifications.state().value())
                : 0,
            modifications.type().is_set(),
            modifications.type().is_set()
                ? static_cast<int32_t>(modifications.type().value())
                : 0,
            modifications.name().is_set(),
            modifications.name().is_set()
                ? rust::String(modifications.name().value())
                : rust::String(""),
            mod_has_parent,
            mod_parent_id,
            modifications.min_width().is_set(),
            modifications.min_width().is_set() ? modifications.min_width().value().as_int() : 0,
            modifications.min_height().is_set(),
            modifications.min_height().is_set() ? modifications.min_height().value().as_int() : 0,
            modifications.max_width().is_set(),
            modifications.max_width().is_set() ? modifications.max_width().value().as_int() : 0,
            modifications.max_height().is_set(),
            modifications.max_height().is_set() ? modifications.max_height().value().as_int() : 0,
            modifications.depth_layer().is_set(),
            modifications.depth_layer().is_set() ? static_cast<int32_t>(modifications.depth_layer().value()) : 0,
            modifications.focus_mode().is_set(),
            modifications.focus_mode().is_set() ? static_cast<int32_t>(modifications.focus_mode().value()) : 0,
            modifications.shell_chrome().is_set(),
            modifications.shell_chrome().is_set() ? static_cast<int32_t>(modifications.shell_chrome().value()) : 0,
            modifications.preferred_orientation().is_set(),
            modifications.preferred_orientation().is_set()
                ? static_cast<int32_t>(modifications.preferred_orientation().value())
                : 0,
            modifications.confine_pointer().is_set(),
            modifications.confine_pointer().is_set()
                ? static_cast<int32_t>(modifications.confine_pointer().value())
                : 0,
            modifications.attached_edges().is_set(),
            modifications.attached_edges().is_set() ? static_cast<uint32_t>(modifications.attached_edges().value()) : 0u,
            modifications.visible_on_lock_screen().is_set(),
            modifications.visible_on_lock_screen().is_set() ? modifications.visible_on_lock_screen().value() : false,
        };

        rust_policy_handle_modify_window(*holder_, info_wrap, spec_data);
    }

    void handle_raise_window(miral::WindowInfo& window_info) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_handle_raise_window(*holder_, info_wrap);
    }

    bool handle_keyboard_event(MirKeyboardEvent const* event) override
    {
        KeyboardEventInfo info{
            static_cast<int32_t>(mir_keyboard_event_action(event)),
            static_cast<int32_t>(mir_keyboard_event_keysym(event)),
            static_cast<int32_t>(mir_keyboard_event_scan_code(event)),
            static_cast<uint32_t>(mir_keyboard_event_modifiers(event)),
            mir_input_event_get_event_time(mir_keyboard_event_input_event(event)),
        };
        return rust_policy_handle_keyboard_event(*holder_, info);
    }

    bool handle_touch_event(MirTouchEvent const* event) override
    {
        // Just report the first touch point for simplicity
        TouchEventInfo info{
            mir_touch_event_point_count(event) > 0
                ? static_cast<int32_t>(mir_touch_event_id(event, 0)) : 0,
            mir_touch_event_point_count(event) > 0
                ? static_cast<int32_t>(mir_touch_event_action(event, 0)) : 0,
            static_cast<uint32_t>(mir_touch_event_modifiers(event)),
            mir_touch_event_point_count(event) > 0
                ? mir_touch_event_axis_value(event, 0, mir_touch_axis_x) : 0.0f,
            mir_touch_event_point_count(event) > 0
                ? mir_touch_event_axis_value(event, 0, mir_touch_axis_y) : 0.0f,
            mir_input_event_get_event_time(mir_touch_event_input_event(event)),
        };
        return rust_policy_handle_touch_event(*holder_, info);
    }

    bool handle_pointer_event(MirPointerEvent const* event) override
    {
        PointerEventInfo info{
            static_cast<int32_t>(mir_pointer_event_action(event)),
            static_cast<uint32_t>(mir_pointer_event_modifiers(event)),
            static_cast<uint32_t>(mir_pointer_event_buttons(event)),
            mir_pointer_event_axis_value(event, mir_pointer_axis_x),
            mir_pointer_event_axis_value(event, mir_pointer_axis_y),
            mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x),
            mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y),
            mir_input_event_get_event_time(mir_pointer_event_input_event(event)),
        };
        return rust_policy_handle_pointer_event(*holder_, info);
    }

    void handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* /*event*/) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_handle_request_move(*holder_, info_wrap);
    }

    void handle_request_resize(
        miral::WindowInfo& window_info,
        MirInputEvent const* /*event*/,
        MirResizeEdge edge) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_handle_request_resize(*holder_, info_wrap, static_cast<int32_t>(edge));
    }

    auto confirm_inherited_move(
        miral::WindowInfo const& window_info,
        mir::geometry::Displacement movement) -> mir::geometry::Rectangle override
    {
        MiralWindowInfo info_wrap{window_info};
        Displacement d{movement.dx.as_int(), movement.dy.as_int()};
        auto result = rust_policy_confirm_inherited_move(*holder_, info_wrap, d);
        return mir::geometry::Rectangle{from_point(result.top_left), from_size(result.size)};
    }

    auto confirm_placement_on_display(
        miral::WindowInfo const& window_info,
        MirWindowState new_state,
        mir::geometry::Rectangle const& new_placement) -> mir::geometry::Rectangle override
    {
        MiralWindowInfo info_wrap{window_info};
        Rectangle rect = to_rectangle(new_placement);
        auto result = rust_policy_confirm_placement_on_display(
            *holder_, info_wrap, static_cast<int32_t>(new_state), rect);
        return mir::geometry::Rectangle{from_point(result.top_left), from_size(result.size)};
    }

    // --- Advisory notifications ---

    void advise_begin() override
    {
        rust_policy_advise_begin(*holder_);
    }

    void advise_end() override
    {
        rust_policy_advise_end(*holder_);
    }

    void advise_new_app(miral::ApplicationInfo& app_info) override
    {
        MiralApplicationInfo app_wrap{app_info};
        rust_policy_advise_new_app(*holder_, app_wrap);
    }

    void advise_delete_app(miral::ApplicationInfo const& app_info) override
    {
        MiralApplicationInfo app_wrap{app_info};
        rust_policy_advise_delete_app(*holder_, app_wrap);
    }

    void advise_new_window(miral::WindowInfo const& window_info) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_new_window(*holder_, info_wrap);
    }

    void advise_delete_window(miral::WindowInfo const& window_info) override
    {
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_delete_window(*holder_, info_wrap);
        // Unregister after notifying Rust
        tools_->unregister_window(window_to_id(window_info.window()));
    }

    void advise_focus_gained(miral::WindowInfo const& window_info) override
    {
        tools_->register_window(window_info.window());
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_focus_gained(*holder_, info_wrap);
    }

    void advise_focus_lost(miral::WindowInfo const& window_info) override
    {
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_focus_lost(*holder_, info_wrap);
    }

    void advise_state_change(miral::WindowInfo const& window_info, MirWindowState state) override
    {
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_state_change(*holder_, info_wrap, static_cast<int32_t>(state));
    }

    void advise_move_to(miral::WindowInfo const& window_info, mir::geometry::Point top_left) override
    {
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_move_to(*holder_, info_wrap, to_point(top_left));
    }

    void advise_resize(miral::WindowInfo const& window_info, mir::geometry::Size const& new_size) override
    {
        MiralWindowInfo info_wrap{window_info};
        rust_policy_advise_resize(*holder_, info_wrap, to_size(new_size));
    }

    void advise_output_create(miral::Output const& output) override
    {
        OutputSnapshot snapshot{
            static_cast<int32_t>(output.id()),
            rust::String(output.name()),
            output.connected(),
            output.used(),
            to_rectangle(output.extents()),
            output.refresh_rate(),
            output.scale(),
            static_cast<int32_t>(output.power_mode()),
            static_cast<int32_t>(output.orientation()),
            static_cast<int32_t>(output.form_factor()),
            static_cast<int32_t>(output.type()),
            output.physical_size_mm().width,
            output.physical_size_mm().height,
        };
        rust_policy_advise_output_create(*holder_, snapshot);
    }

    void advise_output_update(miral::Output const& updated, miral::Output const& original) override
    {
        OutputSnapshot updated_snap{
            static_cast<int32_t>(updated.id()),
            rust::String(updated.name()),
            updated.connected(),
            updated.used(),
            to_rectangle(updated.extents()),
            updated.refresh_rate(),
            updated.scale(),
            static_cast<int32_t>(updated.power_mode()),
            static_cast<int32_t>(updated.orientation()),
            static_cast<int32_t>(updated.form_factor()),
            static_cast<int32_t>(updated.type()),
            updated.physical_size_mm().width,
            updated.physical_size_mm().height,
        };
        OutputSnapshot original_snap{
            static_cast<int32_t>(original.id()),
            rust::String(original.name()),
            original.connected(),
            original.used(),
            to_rectangle(original.extents()),
            original.refresh_rate(),
            original.scale(),
            static_cast<int32_t>(original.power_mode()),
            static_cast<int32_t>(original.orientation()),
            static_cast<int32_t>(original.form_factor()),
            static_cast<int32_t>(original.type()),
            original.physical_size_mm().width,
            original.physical_size_mm().height,
        };
        rust_policy_advise_output_update(*holder_, updated_snap, original_snap);
    }

    void advise_output_delete(miral::Output const& output) override
    {
        OutputSnapshot snapshot{
            static_cast<int32_t>(output.id()),
            rust::String(output.name()),
            output.connected(),
            output.used(),
            to_rectangle(output.extents()),
            output.refresh_rate(),
            output.scale(),
            static_cast<int32_t>(output.power_mode()),
            static_cast<int32_t>(output.orientation()),
            static_cast<int32_t>(output.form_factor()),
            static_cast<int32_t>(output.type()),
            output.physical_size_mm().width,
            output.physical_size_mm().height,
        };
        rust_policy_advise_output_delete(*holder_, snapshot);
    }

    void advise_application_zone_create(miral::Zone const& zone) override
    {
        ZoneSnapshot snapshot{zone.id(), to_rectangle(zone.extents())};
        rust_policy_advise_zone_create(*holder_, snapshot);
    }

    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override
    {
        ZoneSnapshot updated_snap{updated.id(), to_rectangle(updated.extents())};
        ZoneSnapshot original_snap{original.id(), to_rectangle(original.extents())};
        rust_policy_advise_zone_update(*holder_, updated_snap, original_snap);
    }

    void advise_application_zone_delete(miral::Zone const& zone) override
    {
        ZoneSnapshot snapshot{zone.id(), to_rectangle(zone.extents())};
        rust_policy_advise_zone_delete(*holder_, snapshot);
    }

    void advise_adding_to_workspace(
        std::shared_ptr<miral::Workspace> const& /*workspace*/,
        std::vector<miral::Window> const& windows) override
    {
        rust::Vec<uint64_t> ids;
        for (auto const& w : windows)
        {
            tools_->register_window(w);
            ids.push_back(window_to_id(w));
        }
        rust_policy_advise_adding_to_workspace(*holder_, {ids.data(), ids.size()});
    }

    void advise_removing_from_workspace(
        std::shared_ptr<miral::Workspace> const& /*workspace*/,
        std::vector<miral::Window> const& windows) override
    {
        rust::Vec<uint64_t> ids;
        for (auto const& w : windows)
            ids.push_back(window_to_id(w));
        rust_policy_advise_removing_from_workspace(*holder_, {ids.data(), ids.size()});
    }

private:
    std::unique_ptr<MiralTools> tools_;
    rust::Box<RustPolicyHolder> holder_;
};

// --- Runner lifecycle ---

std::unique_ptr<MiralRunner> miral_runner_new(rust::Slice<const rust::String> args)
{
    std::vector<std::string> arg_strings;
    arg_strings.reserve(args.size());

    for (auto const& arg : args)
    {
        arg_strings.emplace_back(std::string(arg));
    }

    return std::make_unique<MiralRunner>(std::move(arg_strings));
}

int32_t miral_runner_run(MiralRunner& runner)
{
    return runner.inner->run_with({});
}

int32_t miral_runner_run_with_rust_policy(MiralRunner& runner)
{
    auto policy_builder = miral::SetWindowManagementPolicy(
        [](miral::WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
        {
            // Call into Rust to create the policy holder
            auto holder = rust_create_policy_holder();
            // Create the policy (which constructs its own MiralTools object)
            auto policy = std::make_unique<RustWindowManagementPolicy>(tools, std::move(holder));
            // Now set the tools pointer to the MiralTools that lives INSIDE the policy.
            // This pointer is valid for the lifetime of the policy object.
            rust_policy_set_tools(policy->holder(), reinterpret_cast<uint64_t>(&policy->miral_tools()));
            return policy;
        });

    return runner.inner->run_with({policy_builder});
}

int32_t miral_runner_run_with_config(
    MiralRunner& runner,
    int32_t decoration_mode,
    rust::Str keymap_layout,
    bool x11_enabled,
    rust::Slice<const ConfigOptionDesc> config_options)
{
    std::vector<std::function<void(mir::Server&)>> options;

    // Policy (always added)
    auto policy_builder = miral::SetWindowManagementPolicy(
        [](miral::WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
        {
            auto holder = rust_create_policy_holder();
            auto policy = std::make_unique<RustWindowManagementPolicy>(tools, std::move(holder));
            rust_policy_set_tools(policy->holder(), reinterpret_cast<uint64_t>(&policy->miral_tools()));
            return policy;
        });
    options.push_back(policy_builder);

    // Decorations
    if (decoration_mode > 0)
    {
        switch (decoration_mode)
        {
        case 1: options.push_back(miral::Decorations::prefer_csd()); break;
        case 2: options.push_back(miral::Decorations::prefer_ssd()); break;
        case 3: options.push_back(miral::Decorations::always_ssd()); break;
        case 4: options.push_back(miral::Decorations::always_csd()); break;
        default: options.push_back(miral::Decorations::prefer_csd()); break;
        }
    }

    // Keymap
    std::string keymap_str(keymap_layout.data(), keymap_layout.size());
    if (!keymap_str.empty())
    {
        miral::Keymap keymap;
        keymap.set_keymap(keymap_str);
        options.push_back(keymap);
    }

    // X11 support
    if (x11_enabled)
    {
        auto x11 = miral::X11Support{}.default_to_enabled();
        options.push_back(x11);
    }

    // Configuration options
    for (auto const& desc : config_options)
    {
        std::string opt_name(desc.name.data(), desc.name.size());
        std::string opt_desc(desc.description.data(), desc.description.size());
        uint32_t cb_id = desc.callback_id;
        bool is_pre_init = desc.pre_init;

        miral::ConfigurationOption config_opt = [&]() -> miral::ConfigurationOption
        {
            switch (desc.option_type)
            {
            case 0: // int with default
                return miral::ConfigurationOption{
                    [cb_id](int value) { rust_config_callback_int(cb_id, value); },
                    opt_name, opt_desc, static_cast<int>(desc.default_int)};
            case 1: // double with default
                return miral::ConfigurationOption{
                    [cb_id](double value) { rust_config_callback_double(cb_id, value); },
                    opt_name, opt_desc, desc.default_double};
            case 2: // string with default
            {
                std::string default_str(desc.default_string.data(), desc.default_string.size());
                return miral::ConfigurationOption{
                    [cb_id](std::string const& value)
                    {
                        rust_config_callback_string(cb_id, rust::Str(value.data(), value.size()));
                    },
                    opt_name, opt_desc, default_str};
            }
            case 3: // bool with default
                return miral::ConfigurationOption{
                    [cb_id](bool value) { rust_config_callback_bool(cb_id, value); },
                    opt_name, opt_desc, desc.default_bool};
            case 4: // flag (presence-only)
                return miral::ConfigurationOption{
                    std::function<void(bool)>{
                        [cb_id](bool is_set) { rust_config_callback_flag(cb_id, is_set); }},
                    opt_name, opt_desc};
            case 5: // optional int
                return miral::ConfigurationOption{
                    [cb_id](mir::optional_value<int> const& value)
                    {
                        rust_config_callback_optional_int(
                            cb_id, value.is_set(),
                            value.is_set() ? value.value() : 0);
                    },
                    opt_name, opt_desc};
            case 6: // optional string
                return miral::ConfigurationOption{
                    [cb_id](mir::optional_value<std::string> const& value)
                    {
                        if (value.is_set())
                        {
                            auto const& str = value.value();
                            rust_config_callback_optional_string(
                                cb_id, true, rust::Str(str.data(), str.size()));
                        }
                        else
                        {
                            rust_config_callback_optional_string(cb_id, false, rust::Str("", 0));
                        }
                    },
                    opt_name, opt_desc};
            case 7: // optional bool
                return miral::ConfigurationOption{
                    [cb_id](mir::optional_value<bool> const& value)
                    {
                        rust_config_callback_optional_bool(
                            cb_id, value.is_set(),
                            value.is_set() ? value.value() : false);
                    },
                    opt_name, opt_desc};
            case 8: // multi-value string list
                return miral::ConfigurationOption{
                    [cb_id](std::vector<std::string> const& values)
                    {
                        rust::Vec<rust::String> rust_values;
                        for (auto const& v : values)
                            rust_values.push_back(rust::String(v));
                        rust_config_callback_multi(cb_id, {rust_values.data(), rust_values.size()});
                    },
                    opt_name, opt_desc};
            default:
                // Unknown type — create a no-op flag option as fallback
                return miral::ConfigurationOption{
                    std::function<void(bool)>{[](bool) {}},
                    opt_name, opt_desc};
            }
        }();

        if (is_pre_init)
            options.push_back(pre_init(config_opt));
        else
            options.push_back(config_opt);
    }

    // External client launcher
    if (runner.has_external_launcher)
    {
        options.push_back(runner.external_launcher);
        runner.inner->add_start_callback([&runner]()
        {
            g_active_launcher.store(&runner.external_launcher);
        });
        runner.inner->add_stop_callback([]()
        {
            g_active_launcher.store(nullptr);
        });
    }

    // Convert vector to a single functor and run
    auto combined = [options = std::move(options)](mir::Server& server)
    {
        for (auto const& opt : options)
            opt(server);
    };

    return runner.inner->run_with({combined});
}

void miral_runner_stop(MiralRunner& runner)
{
    runner.inner->stop();
}

void miral_runner_enable_external_launcher(MiralRunner& runner)
{
    runner.has_external_launcher = true;
}

int32_t miral_launcher_launch(rust::Str command)
{
    auto* launcher = g_active_launcher.load();
    if (!launcher)
        return -1;
    return static_cast<int32_t>(
        launcher->launch(std::string(command.data(), command.size())));
}

void miral_runner_register_start_callback(MiralRunner& runner)
{
    runner.inner->add_start_callback([]()
    {
        rust_on_start_callback();
    });
}

void miral_runner_register_stop_callback(MiralRunner& runner)
{
    runner.inner->add_stop_callback([]()
    {
        rust_on_stop_callback();
    });
}

// --- Window info queries ---

WindowInfoSnapshot miral_window_info_snapshot(MiralWindowInfo const& info)
{
    auto const& i = info.inner;

    auto restore = i.restore_rect();
    auto excl = i.exclusive_rect();
    bool has_excl = excl.is_set();
    bool has_excl_value = has_excl;
    auto clip = i.clip_area();

    uint64_t parent_id = 0;
    if (i.parent())
        parent_id = window_to_id(i.parent());

    return WindowInfoSnapshot{
        rust::String(i.name()),
        static_cast<int32_t>(i.type()),
        static_cast<int32_t>(i.state()),
        to_point(i.window().top_left()),
        to_size(i.window().size()),
        i.min_width().as_int(),
        i.min_height().as_int(),
        i.max_width().as_int(),
        i.max_height().as_int(),
        static_cast<int32_t>(i.depth_layer()),
        static_cast<bool>(i.parent()),
        i.can_be_active(),
        i.is_visible(),
        static_cast<int32_t>(i.focus_mode()),
        to_rectangle(restore),
        i.width_inc().as_int(),
        i.height_inc().as_int(),
        static_cast<int32_t>(i.min_aspect().width),
        static_cast<int32_t>(i.min_aspect().height),
        static_cast<int32_t>(i.max_aspect().width),
        static_cast<int32_t>(i.max_aspect().height),
        i.has_output_id(),
        i.has_output_id() ? i.output_id() : 0,
        static_cast<int32_t>(i.preferred_orientation()),
        static_cast<int32_t>(i.confine_pointer()),
        static_cast<int32_t>(i.shell_chrome()),
        static_cast<uint32_t>(i.attached_edges()),
        has_excl,
        has_excl_value,
        has_excl ? to_rectangle(excl.value()) : Rectangle{Point{0, 0}, Size{0, 0}},
        i.ignore_exclusion_zones(),
        clip.is_set(),
        clip.is_set() ? to_rectangle(clip.value()) : Rectangle{Point{0, 0}, Size{0, 0}},
        rust::String(i.application_id()),
        i.visible_on_lock_screen(),
        static_cast<uint32_t>(i.tiled_edges().value()),
        i.alpha(),
        parent_id,
        i.must_have_parent(),
        i.must_not_have_parent(),
    };
}

std::unique_ptr<MiralWindow> miral_window_info_window(MiralWindowInfo const& info)
{
    return std::make_unique<MiralWindow>(info.inner.window());
}

uint64_t miral_window_info_id(MiralWindowInfo const& info)
{
    return window_to_id(info.inner.window());
}

uint64_t miral_app_info_id(MiralApplicationInfo const& info)
{
    return reinterpret_cast<uint64_t>(info.inner.application().get());
}

// --- Application info queries ---

ApplicationInfoSnapshot miral_app_info_snapshot(MiralApplicationInfo const& info)
{
    auto const& i = info.inner;
    return ApplicationInfoSnapshot{
        rust::String(i.name()),
        static_cast<uint32_t>(i.windows().size()),
    };
}

// --- Window manager tools ---

uint32_t miral_tools_count_applications(MiralTools const& tools)
{
    return tools.inner.count_applications();
}

WindowInfoSnapshot miral_tools_active_window(MiralTools const& tools)
{
    auto window = tools.inner.active_window();
    if (!window)
    {
        return WindowInfoSnapshot{
            rust::String(""),
            0, 0,
            Point{0, 0}, Size{0, 0},
            0, 0, 0, 0,
            0, false, false, false, 0,
            Rectangle{Point{0, 0}, Size{0, 0}},
            0, 0,
            0, 0, 0, 0,
            false, 0,
            0, 0, 0,
            0,
            false, false, Rectangle{Point{0, 0}, Size{0, 0}},
            false,
            false, Rectangle{Point{0, 0}, Size{0, 0}},
            rust::String(""),
            false,
            0,
            0.0f,
            0,
            false, false,
        };
    }
    auto& info = tools.inner.info_for(window);
    return miral_window_info_snapshot(MiralWindowInfo{info});
}

uint64_t miral_tools_active_window_id(MiralTools const& tools)
{
    auto window = tools.inner.active_window();
    if (!window)
        return 0;
    return window_to_id(window);
}

void miral_tools_focus_next_application(MiralTools& tools)
{
    tools.inner.focus_next_application();
}

void miral_tools_focus_prev_application(MiralTools& tools)
{
    tools.inner.focus_prev_application();
}

void miral_tools_focus_next_within_application(MiralTools& tools)
{
    tools.inner.focus_next_within_application();
}

void miral_tools_focus_prev_within_application(MiralTools& tools)
{
    tools.inner.focus_prev_within_application();
}

void miral_tools_raise_tree(MiralTools& tools, MiralWindow const& window)
{
    tools.inner.raise_tree(window.inner);
}

void miral_tools_raise_tree_by_id(MiralTools& tools, uint64_t window_id)
{
    if (auto const* w = tools.lookup_window(window_id))
        tools.inner.raise_tree(*w);
}

void miral_tools_ask_client_to_close_by_id(MiralTools& tools, uint64_t window_id)
{
    if (auto const* w = tools.lookup_window(window_id))
        tools.inner.ask_client_to_close(*w);
}

void miral_tools_modify_window_by_id(MiralTools& tools, uint64_t window_id, WindowSpecData const& spec)
{
    auto const* w = tools.lookup_window(window_id);
    if (!w)
        return;

    miral::WindowSpecification miral_spec;
    if (spec.has_top_left)
        miral_spec.top_left() = from_point(spec.top_left);
    if (spec.has_size)
        miral_spec.size() = from_size(spec.size);
    if (spec.has_state)
        miral_spec.state() = static_cast<MirWindowState>(spec.state);
    if (spec.has_type)
        miral_spec.type() = static_cast<MirWindowType>(spec.window_type);
    if (spec.has_parent)
    {
        if (auto const* parent_w = tools.lookup_window(spec.parent_id))
            miral_spec.parent() = static_cast<std::shared_ptr<mir::scene::Surface>>(*parent_w);
    }
    if (spec.has_min_width)
        miral_spec.min_width() = mir::geometry::Width{spec.min_width};
    if (spec.has_min_height)
        miral_spec.min_height() = mir::geometry::Height{spec.min_height};
    if (spec.has_max_width)
        miral_spec.max_width() = mir::geometry::Width{spec.max_width};
    if (spec.has_max_height)
        miral_spec.max_height() = mir::geometry::Height{spec.max_height};
    if (spec.has_depth_layer)
        miral_spec.depth_layer() = static_cast<MirDepthLayer>(spec.depth_layer);
    if (spec.has_focus_mode)
        miral_spec.focus_mode() = static_cast<MirFocusMode>(spec.focus_mode);
    if (spec.has_shell_chrome)
        miral_spec.shell_chrome() = static_cast<MirShellChrome>(spec.shell_chrome);
    if (spec.has_preferred_orientation)
        miral_spec.preferred_orientation() = static_cast<MirOrientationMode>(spec.preferred_orientation);
    if (spec.has_confine_pointer)
        miral_spec.confine_pointer() = static_cast<MirPointerConfinementState>(spec.confine_pointer);
    if (spec.has_attached_edges)
        miral_spec.attached_edges() = static_cast<MirPlacementGravity>(spec.attached_edges);
    if (spec.has_visible_on_lock_screen)
        miral_spec.visible_on_lock_screen() = spec.visible_on_lock_screen;

    auto& info = tools.inner.info_for(*w);
    tools.inner.modify_window(info, miral_spec);
}

void miral_tools_drag_window_by_id(MiralTools& tools, uint64_t window_id, Displacement movement)
{
    if (auto const* w = tools.lookup_window(window_id))
        tools.inner.drag_window(*w, from_displacement(movement));
}

void miral_tools_select_active_window_by_id(MiralTools& tools, uint64_t window_id)
{
    if (auto const* w = tools.lookup_window(window_id))
        tools.inner.select_active_window(*w);
}

std::unique_ptr<MiralWindow> miral_tools_window_at(MiralTools const& tools, Point point)
{
    auto window = tools.inner.window_at(from_point(point));
    return std::make_unique<MiralWindow>(window);
}

Rectangle miral_tools_active_output(MiralTools const& tools)
{
    auto& non_const_tools = const_cast<MiralTools&>(tools);
    return to_rectangle(non_const_tools.inner.active_output());
}

ZoneSnapshot miral_tools_active_application_zone(MiralTools const& tools)
{
    auto zone = tools.inner.active_application_zone();
    return ZoneSnapshot{zone.id(), to_rectangle(zone.extents())};
}

WindowInfoSnapshot miral_tools_info_for_window_id(MiralTools const& tools, uint64_t window_id)
{
    auto const* w = tools.lookup_window(window_id);
    if (!w)
    {
        // Return empty snapshot for invalid window
        return WindowInfoSnapshot{
            rust::String(""),
            0, 0,
            Point{0, 0}, Size{0, 0},
            0, 0, 0, 0,
            0, false, false, false, 0,
            Rectangle{Point{0, 0}, Size{0, 0}},
            0, 0,
            0, 0, 0, 0,
            false, 0,
            0, 0, 0,
            0,
            false, false, Rectangle{Point{0, 0}, Size{0, 0}},
            false,
            false, Rectangle{Point{0, 0}, Size{0, 0}},
            rust::String(""),
            false,
            0,
            0.0f,
            0,
            false, false,
        };
    }
    auto& info = tools.inner.info_for(*w);
    return miral_window_info_snapshot(MiralWindowInfo{info});
}

void miral_tools_swap_tree_order_by_id(MiralTools& tools, uint64_t window_id_a, uint64_t window_id_b)
{
    auto const* wa = tools.lookup_window(window_id_a);
    auto const* wb = tools.lookup_window(window_id_b);
    if (wa && wb)
        tools.inner.swap_tree_order(*wa, *wb);
}

void miral_tools_send_tree_to_back_by_id(MiralTools& tools, uint64_t window_id)
{
    if (auto const* w = tools.lookup_window(window_id))
        tools.inner.send_tree_to_back(*w);
}

void miral_tools_drag_active_window(MiralTools& tools, Displacement movement)
{
    tools.inner.drag_active_window(from_displacement(movement));
}

void miral_tools_move_cursor_to(MiralTools& tools, Point point)
{
    tools.inner.move_cursor_to(mir::geometry::PointF{
        static_cast<float>(point.x), static_cast<float>(point.y)});
}

rust::Vec<uint64_t> miral_tools_all_window_ids(MiralTools const& tools)
{
    rust::Vec<uint64_t> ids;
    // Use for_each_application to iterate all windows across all apps
    const_cast<miral::WindowManagerTools&>(tools.inner).for_each_application(
        [&](miral::ApplicationInfo& app_info)
        {
            for (auto const& window : app_info.windows())
            {
                auto id = window_to_id(window);
                if (id != 0)
                    ids.push_back(id);
            }
        });
    return ids;
}

uint64_t miral_tools_window_id_at(MiralTools const& tools, Point point)
{
    auto window = tools.inner.window_at(from_point(point));
    if (!window)
        return 0;
    return window_to_id(window);
}

Rectangle miral_tools_place_and_size_for_state(
    MiralTools const& tools,
    uint64_t window_id,
    int32_t new_state,
    Rectangle const& rect)
{
    auto const* w = tools.lookup_window(window_id);
    if (!w)
        return rect;
    auto& info = tools.inner.info_for(*w);
    miral::WindowSpecification spec;
    spec.state() = static_cast<MirWindowState>(new_state);
    spec.top_left() = from_point(rect.top_left);
    spec.size() = from_size(rect.size);
    tools.inner.place_and_size_for_state(spec, info);
    // Return the placement from the spec (which was modified in-place)
    auto result_top_left = spec.top_left().is_set() ? spec.top_left().value() : from_point(rect.top_left);
    auto result_size = spec.size().is_set() ? spec.size().value() : from_size(rect.size);
    return to_rectangle(mir::geometry::Rectangle{result_top_left, result_size});
}

// --- Window queries ---

Point miral_window_top_left(MiralWindow const& window)
{
    return to_point(window.inner.top_left());
}

Size miral_window_size(MiralWindow const& window)
{
    return to_size(window.inner.size());
}

bool miral_window_is_valid(MiralWindow const& window)
{
    return static_cast<bool>(window.inner);
}

uint64_t miral_window_id(MiralWindow const& window)
{
    return window_to_id(window.inner);
}

} // namespace miral_sys
