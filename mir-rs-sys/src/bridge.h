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

#pragma once

#include "rust/cxx.h"
#include <memory>
#include <unordered_map>

#include <miral/runner.h>
#include <miral/window.h>
#include <miral/window_info.h>
#include <miral/application_info.h>
#include <miral/window_manager_tools.h>
#include <miral/external_client.h>

namespace mir_sys
{

// Forward declarations for types defined in the CXX bridge
struct Point;
struct Size;
struct Rectangle;
struct Displacement;
struct KeyboardEventInfo;
struct PointerEventInfo;
struct TouchEventInfo;
struct WindowInfoSnapshot;
struct ApplicationInfoSnapshot;
struct OutputSnapshot;
struct ZoneSnapshot;
struct WindowSpecData;
struct ConfigOptionDesc;
struct RustPolicyHolder;

// Opaque C++ types exposed to Rust — wrapping the actual miral types
class MiralWindow
{
public:
    miral::Window inner;
    MiralWindow() = default;
    explicit MiralWindow(miral::Window w) : inner(std::move(w)) {}
};

class MiralWindowInfo
{
public:
    miral::WindowInfo const& inner;
    explicit MiralWindowInfo(miral::WindowInfo const& i) : inner(i) {}
};

class MiralApplicationInfo
{
public:
    miral::ApplicationInfo const& inner;
    explicit MiralApplicationInfo(miral::ApplicationInfo const& i) : inner(i) {}
};

class MiralTools
{
public:
    miral::WindowManagerTools inner;
    // Registry mapping window IDs to miral::Window objects
    std::unordered_map<uint64_t, miral::Window> window_registry;

    explicit MiralTools(miral::WindowManagerTools t) : inner(std::move(t)) {}

    // Register a window so it can be looked up by ID later
    void register_window(miral::Window const& window);
    // Unregister a window
    void unregister_window(uint64_t id);
    // Look up a window by ID (returns nullptr if not found)
    miral::Window const* lookup_window(uint64_t id) const;
};

class MiralRunner
{
public:
    // Argument storage — must outlive `inner` since MirRunner stores argv pointers
    std::vector<std::string> arg_strings;
    std::vector<char const*> argv;
    std::unique_ptr<miral::MirRunner> inner;
    miral::ExternalClientLauncher external_launcher;
    bool has_external_launcher = false;

    // Accumulated server options — each extension pushes its functor here
    std::vector<std::function<void(mir::Server&)>> options;

    MiralRunner(std::vector<std::string> args)
        : arg_strings(std::move(args))
    {
        argv.reserve(arg_strings.size());
        for (auto const& s : arg_strings)
            argv.push_back(s.c_str());
        inner = std::make_unique<miral::MirRunner>(
            static_cast<int>(argv.size()), argv.data());
    }

    MiralRunner(MiralRunner const&) = delete;
    MiralRunner& operator=(MiralRunner const&) = delete;
};

// Runner lifecycle
std::unique_ptr<MiralRunner> miral_runner_new(rust::Slice<const rust::String> args);
int32_t miral_runner_run(MiralRunner& runner);
int32_t miral_runner_run_with_rust_policy(MiralRunner& runner);
int32_t miral_runner_run_with_config(
    MiralRunner& runner,
    rust::Slice<const ConfigOptionDesc> config_options);
void miral_runner_stop(MiralRunner& runner);
void miral_runner_register_start_callback(MiralRunner& runner);
void miral_runner_register_stop_callback(MiralRunner& runner);

// Extension adders — each constructs a miral type and pushes it into runner.options
void miral_runner_add_decorations(MiralRunner& runner, int32_t mode);
void miral_runner_add_keymap(MiralRunner& runner, rust::Str layout);
void miral_runner_add_x11_support(MiralRunner& runner);
void miral_runner_add_external_launcher(MiralRunner& runner);
void miral_runner_add_idle_listener(MiralRunner& runner);
void miral_runner_add_session_lock_listener(MiralRunner& runner);
void miral_runner_add_magnifier(
    MiralRunner& runner,
    float magnification,
    int32_t width,
    int32_t height,
    bool enabled);
void miral_runner_add_bounce_keys(MiralRunner& runner, bool enabled);
void miral_runner_add_slow_keys(MiralRunner& runner, bool enabled);
void miral_runner_add_sticky_keys(MiralRunner& runner, bool enabled);
void miral_runner_add_mousekeys(MiralRunner& runner, bool enabled);
void miral_runner_add_locate_pointer(MiralRunner& runner, bool enabled);
void miral_runner_add_cursor_theme(MiralRunner& runner, rust::Str theme);
void miral_runner_add_cursor_scale(MiralRunner& runner, float scale);
void miral_runner_add_input_configuration(MiralRunner& runner);
void miral_runner_add_display_configuration(MiralRunner& runner);
void miral_runner_add_output_filter(MiralRunner& runner);
void miral_runner_add_init_callback(MiralRunner& runner);
void miral_runner_add_terminator(MiralRunner& runner);
int32_t miral_launcher_launch(rust::Str command);

// Window info queries
WindowInfoSnapshot miral_window_info_snapshot(MiralWindowInfo const& info);
std::unique_ptr<MiralWindow> miral_window_info_window(MiralWindowInfo const& info);
uint64_t miral_window_info_id(MiralWindowInfo const& info);
uint64_t miral_app_info_id(MiralApplicationInfo const& info);

// Application info queries
ApplicationInfoSnapshot miral_app_info_snapshot(MiralApplicationInfo const& info);

// Window manager tools (ID-based for Rust interop)
uint32_t miral_tools_count_applications(MiralTools const& tools);
WindowInfoSnapshot miral_tools_active_window(MiralTools const& tools);
uint64_t miral_tools_active_window_id(MiralTools const& tools);
void miral_tools_focus_next_application(MiralTools& tools);
void miral_tools_focus_prev_application(MiralTools& tools);
void miral_tools_focus_next_within_application(MiralTools& tools);
void miral_tools_focus_prev_within_application(MiralTools& tools);
void miral_tools_raise_tree(MiralTools& tools, MiralWindow const& window);
void miral_tools_raise_tree_by_id(MiralTools& tools, uint64_t window_id);
void miral_tools_ask_client_to_close_by_id(MiralTools& tools, uint64_t window_id);
void miral_tools_modify_window_by_id(MiralTools& tools, uint64_t window_id, WindowSpecData const& spec);
void miral_tools_drag_window_by_id(MiralTools& tools, uint64_t window_id, Displacement movement);
void miral_tools_select_active_window_by_id(MiralTools& tools, uint64_t window_id);
std::unique_ptr<MiralWindow> miral_tools_window_at(MiralTools const& tools, Point point);
Rectangle miral_tools_active_output(MiralTools const& tools);
ZoneSnapshot miral_tools_active_application_zone(MiralTools const& tools);

// New tools functions
WindowInfoSnapshot miral_tools_info_for_window_id(MiralTools const& tools, uint64_t window_id);
void miral_tools_swap_tree_order_by_id(MiralTools& tools, uint64_t window_id_a, uint64_t window_id_b);
void miral_tools_send_tree_to_back_by_id(MiralTools& tools, uint64_t window_id);
void miral_tools_drag_active_window(MiralTools& tools, Displacement movement);
void miral_tools_move_cursor_to(MiralTools& tools, Point point);
rust::Vec<uint64_t> miral_tools_all_window_ids(MiralTools const& tools);
uint64_t miral_tools_window_id_at(MiralTools const& tools, Point point);
Rectangle miral_tools_place_and_size_for_state(
    MiralTools const& tools,
    uint64_t window_id,
    int32_t new_state,
    Rectangle const& rect);

// Window queries
Point miral_window_top_left(MiralWindow const& window);
Size miral_window_size(MiralWindow const& window);
bool miral_window_is_valid(MiralWindow const& window);
uint64_t miral_window_id(MiralWindow const& window);

} // namespace mir_sys
