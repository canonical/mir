/*
 * Copyright © 2016 Canonical Ltd.
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

#include "window_management_trace.h"
#include "window_info_defaults.h"

#include <miral/application_info.h>
#include <miral/output.h>
#include <miral/zone.h>
#include <miral/window_info.h>

#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/event_printer.h>

#include <iomanip>
#include <sstream>

#define MIR_LOG_COMPONENT "miral::Window Management"
#include <mir/log.h>
#include <mir/report_exception.h>

using mir::operator<<;

#define MIRAL_TRACE_EXCEPTION \
catch (std::exception const& x)\
{\
    std::stringstream out;\
    mir::report_exception(out);\
    mir::log_warning("%s throws %s", __func__, out.str().c_str());\
    throw;\
}


namespace
{
std::string const null_ptr{"(null)"};

auto operator<< (std::ostream& out, miral::WindowSpecification::AspectRatio const& ratio) -> std::ostream&;

struct BracedItemStream
{
    BracedItemStream(std::ostream& out) : out{out} { out << '{'; }
    ~BracedItemStream() { out << '}'; }
    bool mutable first_field = true;
    std::ostream& out;

    template<typename Type>
    auto append(Type const& item) const -> BracedItemStream const&
    {
        if (!first_field) out << ", ";
        out << item;
        first_field = false;
        return *this;
    }

    template<typename Type>
    auto append(char const* name, Type const& item) const -> BracedItemStream const&
    {
        if (!first_field) out << ", ";
        out << name << '=' << item;
        first_field = false;
        return *this;
    }

    auto append(char const* name, MirOrientationMode item) const -> BracedItemStream const&
    {
        auto const flags = out.flags();
        auto const prec  = out.precision();
        auto const fill  = out.fill();

        if (!first_field) out << ", ";
        out << name << '=' << std::showbase << std::internal << std::setfill('0') << std::setw(2) << std::hex << item;
        first_field = false;

        out.flags(flags);
        out.precision(prec);
        out.fill(fill);
        return *this;
    }
};

auto operator<< (std::ostream& out, miral::WindowSpecification::AspectRatio const& ratio) -> std::ostream&
{
    BracedItemStream{out}.append(ratio.width).append(ratio.height);
    return out;
}

auto dump_of(miral::Window const& window) -> std::string
{
    if (std::shared_ptr<mir::scene::Surface> surface = window)
        return surface->name();
    else
        return null_ptr;
}

auto dump_of(miral::Application const& application) -> std::string
{
    if (application)
        return application->name();
    else
        return null_ptr;
}

auto dump_of(std::vector<miral::Window> const& windows) -> std::string;

inline auto operator!=(miral::WindowInfo::AspectRatio const& lhs, miral::WindowInfo::AspectRatio const& rhs)
{
    return lhs.width != rhs.width || lhs.height != rhs.height;
}

auto dump_of(miral::WindowInfo const& info) -> std::string
{
    using namespace mir::geometry;

    std::stringstream out;
    {
        BracedItemStream bout{out};

#define APPEND(field) bout.append(#field, info.field());
        APPEND(name);
        APPEND(type);
        APPEND(state);
        bout.append("top_left", info.window().top_left());
        bout.append("size", info.window().size());
        if (info.state() != mir_window_state_restored) APPEND(restore_rect);
        if (std::shared_ptr<mir::scene::Surface> parent = info.parent())
            bout.append("parent", parent->name());
        bout.append("children", dump_of(info.children()));
        if (info.min_width()  != miral::default_min_width)  APPEND(min_width);
        if (info.min_height() != miral::default_min_height) APPEND(min_height);
        if (info.max_width()  != miral::default_max_width)  APPEND(max_width);
        if (info.max_height() != miral::default_max_height) APPEND(max_height);
        if (info.width_inc()  != miral::default_width_inc)  APPEND(width_inc);
        if (info.height_inc() != miral::default_height_inc) APPEND(height_inc);
        if (info.min_aspect() != miral::default_min_aspect_ratio) APPEND(min_aspect);
        if (info.max_aspect() != miral::default_max_aspect_ratio) APPEND(max_aspect);
        if (info.depth_layer() != mir_depth_layer_application) APPEND(depth_layer);
        if (info.attached_edges() != 0) APPEND(attached_edges);
        if (info.exclusive_rect() != mir::geometry::Rectangle{{}, {}}) APPEND(exclusive_rect);
        APPEND(preferred_orientation);
        APPEND(confine_pointer);

#define APPEND_IF_SET(field) if (info.has_##field()) bout.append(#field, info.field());
        APPEND_IF_SET(output_id);
#undef  APPEND_IF_SET
#undef  APPEND
    }

    return out.str();
}

auto dump_of(miral::WindowSpecification const& specification) -> std::string
{
    std::stringstream out;

    {
        BracedItemStream bout{out};

#define APPEND_IF_SET(field) if (specification.field().is_set()) bout.append(#field, specification.field().value());
        APPEND_IF_SET(name);
        APPEND_IF_SET(type);
        APPEND_IF_SET(top_left);
        APPEND_IF_SET(size);
        APPEND_IF_SET(output_id);
        APPEND_IF_SET(state);
        APPEND_IF_SET(preferred_orientation);
        APPEND_IF_SET(aux_rect);
        APPEND_IF_SET(placement_hints);
        APPEND_IF_SET(window_placement_gravity);
        APPEND_IF_SET(aux_rect_placement_gravity);
        APPEND_IF_SET(aux_rect_placement_offset);
        APPEND_IF_SET(min_width);
        APPEND_IF_SET(min_height);
        APPEND_IF_SET(max_width);
        APPEND_IF_SET(max_height);
        APPEND_IF_SET(width_inc);
        APPEND_IF_SET(height_inc);
        APPEND_IF_SET(min_aspect);
        APPEND_IF_SET(max_aspect);
        if (specification.parent().is_set())
            if (auto const& parent = specification.parent().value().lock())
                bout.append("parent", parent->name());
//        APPEND_IF_SET(input_shape);
//        APPEND_IF_SET(input_mode);
        APPEND_IF_SET(shell_chrome);
        APPEND_IF_SET(confine_pointer);
        APPEND_IF_SET(depth_layer);
        APPEND_IF_SET(attached_edges);
        APPEND_IF_SET(exclusive_rect);
#undef  APPEND_IF_SET
    }

    return out.str();
}

auto dump_of(std::vector<miral::Window> const& windows) -> std::string
{
    std::stringstream out;

    {
        BracedItemStream bout{out};

        for (auto const& window: windows)
            bout.append(dump_of(window));
    }

    return out.str();
}

auto dump_of(miral::ApplicationInfo const& app_info) -> std::string
{
    std::stringstream out;

    BracedItemStream{out}
        .append("application", dump_of(app_info.application()))
        .append("windows", dump_of(app_info.windows()));

    return out.str();
}

auto dump_of(MirKeyboardEvent const* event) -> std::string
{
    std::stringstream out;

    auto device_id = mir_input_event_get_device_id(mir_keyboard_event_input_event(event));

    {
        BracedItemStream bout{out};

        bout.append("from", device_id)
            .append("action", mir_keyboard_event_action(event))
            .append("code", mir_keyboard_event_key_code(event))
            .append("scan", mir_keyboard_event_scan_code(event));

        out.setf(std::ios_base::hex, std::ios_base::basefield);
        bout.append(std::hex).append("modifiers", mir_keyboard_event_modifiers(event));
    }

    return out.str();
}

auto dump_of(MirTouchEvent const* event) -> std::string
{
    std::stringstream out;

    auto device_id = mir_input_event_get_device_id(mir_touch_event_input_event(event));

    {
        BracedItemStream bout{out};

        bout.append("from", device_id);

        for (unsigned int index = 0, count = mir_touch_event_point_count(event); index != count; ++index)
        {
            BracedItemStream{out}
                .append("id", mir_touch_event_id(event, index))
                .append("action", mir_touch_event_action(event, index))
                .append("tool", mir_touch_event_tooltype(event, index))
                .append("x", mir_touch_event_axis_value(event, index, mir_touch_axis_x))
                .append("y", mir_touch_event_axis_value(event, index, mir_touch_axis_y))
                .append("pressure", mir_touch_event_axis_value(event, index, mir_touch_axis_pressure))
                .append("major", mir_touch_event_axis_value(event, index, mir_touch_axis_touch_major))
                .append("minor", mir_touch_event_axis_value(event, index, mir_touch_axis_touch_minor))
                .append("size", mir_touch_event_axis_value(event, index, mir_touch_axis_size));
        }

        out.setf(std::ios_base::hex, std::ios_base::basefield);
        bout.append("modifiers", mir_touch_event_modifiers(event));
    }

    return out.str();
}

auto dump_of(MirPointerEvent const* event) -> std::string
{
    std::stringstream out;

    auto device_id = mir_input_event_get_device_id(mir_pointer_event_input_event(event));

    unsigned int button_state = 0;

    for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
                         mir_pointer_button_back, mir_pointer_button_forward})
        button_state |= mir_pointer_event_button_state(event, a) ? a : 0;

    {
        BracedItemStream bout{out};

        bout.append("from", device_id)
            .append("action", mir_pointer_event_action(event))
            .append("button_state", button_state)
            .append("x", mir_pointer_event_axis_value(event, mir_pointer_axis_x))
            .append("y", mir_pointer_event_axis_value(event, mir_pointer_axis_y))
            .append("dx", mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x))
            .append("dy", mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y))
            .append("vscroll", mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll))
            .append("hscroll", mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll));

        out.setf(std::ios_base::hex, std::ios_base::basefield);
        bout.append("modifiers", mir_pointer_event_modifiers(event));
    }

    return out.str();
}

auto dump_of(MirWindowState state) -> std::string
{
    std::stringstream out;
    out << state;
    return out.str();
}

auto dump_of(mir::geometry::Point point) -> std::string
{
    std::stringstream out;
    out << point;
    return out.str();
}

auto dump_of(mir::geometry::Size const size) -> std::string
{
    std::stringstream out;
    out << size;
    return out.str();
}

auto dump_of(mir::geometry::Rectangle const& rect) -> std::string
{
    std::stringstream out;
    out << rect;
    return out.str();
}

auto dump_of(miral::Output const& output) -> std::string
{
    return dump_of(output.extents());
}

auto dump_of(miral::Zone const& zone) -> std::string
{
    return dump_of(zone.extents());
}
}

miral::WindowManagementTrace::WindowManagementTrace(
    WindowManagerTools const& wrapped,
    WindowManagementPolicyBuilder const& builder) :
    wrapped{wrapped},
    policy(builder(WindowManagerTools{this})),
    policy_application_zone_addendum{WindowManagementPolicy::ApplicationZoneAddendum::from(policy.get())}
{
}

auto miral::WindowManagementTrace::count_applications() const -> unsigned int
try {
    log_input();
    auto const result = wrapped.count_applications();
    mir::log_info("%s -> %d", __func__, result);
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::for_each_application(std::function<void(miral::ApplicationInfo&)> const& functor)
try {
    log_input();
    mir::log_info("%s", __func__);
    trace_count++;
    wrapped.for_each_application(functor);
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
-> Application
try {
    log_input();
    auto result = wrapped.find_application(predicate);
    mir::log_info("%s -> %s", __func__, dump_of(result).c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo&
try {
    log_input();
    auto& result = wrapped.info_for(session);
    mir::log_info("%s -> %s", __func__, result.application()->name().c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo&
try {
    log_input();
    auto& result = wrapped.info_for(surface);
    mir::log_info("%s -> %s", __func__, result.name().c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::info_for(Window const& window) const -> WindowInfo&
try {
    log_input();
    auto& result = wrapped.info_for(window);
    mir::log_info("%s -> %s", __func__, result.name().c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::ask_client_to_close(miral::Window const& window)
try {
    log_input();
    mir::log_info("%s -> %s", __func__, dump_of(window).c_str());
    trace_count++;
    wrapped.ask_client_to_close(window);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::force_close(miral::Window const& window)
try {
    log_input();
    mir::log_info("%s -> %s", __func__, dump_of(window).c_str());
    trace_count++;
    wrapped.force_close(window);
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::active_window() const -> Window
try {
    log_input();
    auto result = wrapped.active_window();
    mir::log_info("%s -> %s", __func__, dump_of(result).c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::select_active_window(Window const& hint) -> Window
try {
    log_input();
    auto result = wrapped.select_active_window(hint);
    mir::log_info("%s hint=%s -> %s", __func__, dump_of(hint).c_str(), dump_of(result).c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::window_at(mir::geometry::Point cursor) const -> Window
try {
    log_input();
    auto result = wrapped.window_at(cursor);
    std::stringstream out;
    out << cursor << " -> " << dump_of(result);
    mir::log_info("%s cursor=%s", __func__, out.str().c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::active_output() -> mir::geometry::Rectangle const
try {
    log_input();
    auto result = wrapped.active_output();
    std::stringstream out;
    out << result;
    mir::log_info("%s -> ", __func__, out.str().c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::info_for_window_id(std::string const& id) const -> WindowInfo&
try {
    log_input();
    auto& result = wrapped.info_for_window_id(id);
    mir::log_info("%s id=%s -> %s", __func__, id.c_str(), dump_of(result).c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::id_for_window(Window const& window) const -> std::string
try {
    log_input();
    auto result = wrapped.id_for_window(window);
    mir::log_info("%s window=%s -> %s", __func__, dump_of(window).c_str(), result.c_str());
    trace_count++;
    return result;
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::place_and_size_for_state(
    WindowSpecification& modifications, WindowInfo const& window_info) const
try {
    log_input();
    mir::log_info("%s modifications=%s window_info=%s", __func__, dump_of(modifications).c_str(), dump_of(window_info).c_str());
    wrapped.place_and_size_for_state(modifications, window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::drag_active_window(mir::geometry::Displacement movement)
try {
    log_input();
    std::stringstream out;
    out << movement;
    mir::log_info("%s movement=%s", __func__, out.str().c_str());
    trace_count++;
    wrapped.drag_active_window(movement);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::drag_window(Window const& window, mir::geometry::Displacement& movement)
try {
    log_input();
    std::stringstream out;
    out << movement;
    mir::log_info("%s window=%s -> %s", __func__, dump_of(window).c_str(), out.str().c_str());
    trace_count++;
    wrapped.drag_window(window, movement);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::focus_next_application()
try {
    log_input();
    mir::log_info("%s", __func__);
    trace_count++;
    wrapped.focus_next_application();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::focus_prev_application()
try {
    log_input();
    mir::log_info("%s", __func__);
    trace_count++;
    wrapped.focus_next_application();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::focus_next_within_application()
try {
    log_input();
    mir::log_info("%s", __func__);
    trace_count++;
    wrapped.focus_next_within_application();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::focus_prev_within_application()
try {
    log_input();
    mir::log_info("%s", __func__);
    trace_count++;
    wrapped.focus_prev_within_application();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::raise_tree(miral::Window const& root)
try {
    log_input();
    mir::log_info("%s root=%s", __func__, dump_of(root).c_str());
    trace_count++;
    wrapped.raise_tree(root);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::start_drag_and_drop(miral::WindowInfo& window_info, std::vector<uint8_t> const& handle)
try {
    log_input();
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    trace_count++;
    wrapped.start_drag_and_drop(window_info, handle);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::end_drag_and_drop()
try {
    log_input();
    mir::log_info("%s window_info=%s", __func__);
    trace_count++;
    wrapped.end_drag_and_drop();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::modify_window(
    miral::WindowInfo& window_info, miral::WindowSpecification const& modifications)
try {
    log_input();
    mir::log_info("%s window_info=%s, modifications=%s",
                  __func__, dump_of(window_info).c_str(), dump_of(modifications).c_str());
    trace_count++;
    wrapped.modify_window(window_info, modifications);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::invoke_under_lock(std::function<void()> const& callback)
try {
    mir::log_info("%s", __func__);
    wrapped.invoke_under_lock(callback);
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::create_workspace() -> std::shared_ptr<Workspace>
try {
    mir::log_info("%s", __func__);
    return wrapped.create_workspace();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::add_tree_to_workspace(
    miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace)
try {
    mir::log_info("%s window=%s, workspace =%p", __func__, dump_of(window).c_str(), workspace.get());
    wrapped.add_tree_to_workspace(window, workspace);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::remove_tree_from_workspace(
    miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace)
try {
    mir::log_info("%s window=%s, workspace =%p", __func__, dump_of(window).c_str(), workspace.get());
    wrapped.remove_tree_from_workspace(window, workspace);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::move_workspace_content_to_workspace(
    std::shared_ptr<Workspace> const& to_workspace, std::shared_ptr<Workspace> const& from_workspace)
try {
    mir::log_info("%s to_workspace=%p, from_workspace=%p", __func__, to_workspace.get(), from_workspace.get());
    wrapped.move_workspace_content_to_workspace(to_workspace, from_workspace);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::for_each_workspace_containing(
    miral::Window const& window, std::function<void(std::shared_ptr<miral::Workspace> const&)> const& callback)
try {
    mir::log_info("%s window=%s", __func__, dump_of(window).c_str());
    wrapped.for_each_workspace_containing(window, callback);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::for_each_window_in_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::function<void(miral::Window const&)> const& callback)
try {
    mir::log_info("%s workspace =%p", __func__, workspace.get());
    wrapped.for_each_window_in_workspace(workspace, callback);
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::place_new_window(
    ApplicationInfo const& app_info,
    WindowSpecification const& requested_specification) -> WindowSpecification
try {
    auto const result = policy->place_new_window(app_info, requested_specification);
    mir::log_info("%s app_info=%s, requested_specification=%s -> %s",
              __func__, dump_of(app_info).c_str(), dump_of(requested_specification).c_str(), dump_of(result).c_str());
    return result;
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_window_ready(miral::WindowInfo& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->handle_window_ready(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_modify_window(
    miral::WindowInfo& window_info, miral::WindowSpecification const& modifications)
try {
    mir::log_info("%s window_info=%s, modifications=%s",
                  __func__, dump_of(window_info).c_str(), dump_of(modifications).c_str());
    policy->handle_modify_window(window_info, modifications);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_raise_window(miral::WindowInfo& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->handle_raise_window(window_info);
}
MIRAL_TRACE_EXCEPTION

bool miral::WindowManagementTrace::handle_keyboard_event(MirKeyboardEvent const* event)
try {
    log_input = [event, this]
        {
            mir::log_info("handle_keyboard_event event=%s", dump_of(event).c_str());
            log_input = []{};
        };

    return policy->handle_keyboard_event(event);
}
MIRAL_TRACE_EXCEPTION

bool miral::WindowManagementTrace::handle_touch_event(MirTouchEvent const* event)
try {
    log_input = [event, this]
        {
            mir::log_info("handle_touch_event event=%s", dump_of(event).c_str());
            log_input = []{};
        };

    return policy->handle_touch_event(event);
}
MIRAL_TRACE_EXCEPTION

bool miral::WindowManagementTrace::handle_pointer_event(MirPointerEvent const* event)
try {
    log_input = [event, this]
        {
            mir::log_info("handle_pointer_event event=%s", dump_of(event).c_str());
            log_input = []{};
        };

    return policy->handle_pointer_event(event);
}
MIRAL_TRACE_EXCEPTION

auto miral::WindowManagementTrace::confirm_inherited_move(WindowInfo const& window_info, Displacement movement)
-> Rectangle
try {
    std::stringstream out;
    out << movement;
    mir::log_info("%s window_info=%s, movement=%s", __func__, dump_of(window_info).c_str(), out.str().c_str());

    return policy->confirm_inherited_move(window_info, movement);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_begin()
try {
    log_input = []{};
    trace_count.store(0);
    policy->advise_begin();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_end()
try {
    if (trace_count.load() > 0)
        mir::log_info("====");
    policy->advise_end();
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_new_app(miral::ApplicationInfo& application)
try {
    mir::log_info("%s application=%s", __func__, dump_of(application).c_str());
    policy->advise_new_app(application);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_delete_app(miral::ApplicationInfo const& application)
try {
    mir::log_info("%s application=%s", __func__, dump_of(application).c_str());
    policy->advise_delete_app(application);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_new_window(miral::WindowInfo const& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->advise_new_window(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_focus_lost(miral::WindowInfo const& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->advise_focus_lost(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_focus_gained(miral::WindowInfo const& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->advise_focus_gained(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_state_change(miral::WindowInfo const& window_info, MirWindowState state)
try {
    mir::log_info("%s window_info=%s, state=%s", __func__, dump_of(window_info).c_str(), dump_of(state).c_str());
    policy->advise_state_change(window_info, state);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_move_to(miral::WindowInfo const& window_info, mir::geometry::Point top_left)
try {
    mir::log_info("%s window_info=%s, top_left=%s", __func__, dump_of(window_info).c_str(), dump_of(top_left).c_str());
    policy->advise_move_to(window_info, top_left);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_resize(miral::WindowInfo const& window_info, mir::geometry::Size const& new_size)
try {
    mir::log_info("%s window_info=%s, new_size=%s", __func__, dump_of(window_info).c_str(), dump_of(new_size).c_str());
    policy->advise_resize(window_info, new_size);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_delete_window(miral::WindowInfo const& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->advise_delete_window(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_raise(std::vector<miral::Window> const& windows)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(windows).c_str());
    policy->advise_raise(windows);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_request_drag_and_drop(miral::WindowInfo& window_info)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->handle_request_drag_and_drop(window_info);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event)
try {
    mir::log_info("%s window_info=%s", __func__, dump_of(window_info).c_str());
    policy->handle_request_move(window_info, input_event);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::handle_request_resize(
    miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
try {
    mir::log_info("%s window_info=%s, edge=0x%1x", __func__, dump_of(window_info).c_str(), edge);
    policy->handle_request_resize(window_info, input_event, edge);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_adding_to_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::vector<miral::Window> const& windows)
try {
    mir::log_info("%s workspace=%p, windows=%s", __func__, workspace.get(), dump_of(windows).c_str());
    policy->advise_adding_to_workspace(workspace, windows);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_removing_from_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::vector<miral::Window> const& windows)
try {
    mir::log_info("%s workspace=%p, windows=%s", __func__, workspace.get(), dump_of(windows).c_str());
    policy->advise_removing_from_workspace(workspace, windows);
}
MIRAL_TRACE_EXCEPTION


auto miral::WindowManagementTrace::confirm_placement_on_display(
    WindowInfo const& window_info,
    MirWindowState new_state,
    Rectangle const& new_placement) -> Rectangle
try {
    auto const& result = policy->confirm_placement_on_display(window_info, new_state, new_placement);
    mir::log_info("%s window_info=%s, new_state= %s, new_placement= %s -> %s", __func__,
        dump_of(window_info).c_str(), dump_of(new_state).c_str(), dump_of(new_placement).c_str(), dump_of(result).c_str());
    return result;
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_output_create(Output const& output)
try {
    mir::log_info("%s output=%s", __func__, dump_of(output).c_str());
    return policy->advise_output_create(output);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_output_update(Output const& updated, Output const& original)
try {
    mir::log_info("%s updated=%s, original=%s", __func__, dump_of(updated).c_str(), dump_of(original).c_str());
    return policy->advise_output_update(updated, original);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_output_delete(Output const& output)
try {
    mir::log_info("%s output=%s", __func__, dump_of(output).c_str());
    return policy->advise_output_delete(output);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_application_zone_create(Zone const& application_zone)
try {
    mir::log_info("%s application_zone=%s", __func__, dump_of(application_zone).c_str());
    return policy_application_zone_addendum->advise_application_zone_create(application_zone);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_application_zone_update(Zone const& updated, Zone const& original)
try {
    mir::log_info("%s updated=%s, original=%s", __func__, dump_of(updated).c_str(), dump_of(original).c_str());
    return policy_application_zone_addendum->advise_application_zone_update(updated, original);
}
MIRAL_TRACE_EXCEPTION

void miral::WindowManagementTrace::advise_application_zone_delete(Zone const& application_zone)
try {
    mir::log_info("%s application_zone=%s", __func__, dump_of(application_zone).c_str());
    return policy_application_zone_addendum->advise_application_zone_delete(application_zone);
}
MIRAL_TRACE_EXCEPTION
