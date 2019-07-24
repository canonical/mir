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

#ifndef MIRAL_WINDOW_INFO_H
#define MIRAL_WINDOW_INFO_H

#include "miral/window.h"
#include "miral/window_specification.h"

#include <mir/geometry/rectangles.h>
#include <mir/optional_value.h>

#include <algorithm>

namespace miral
{
struct WindowInfo
{
    using AspectRatio = WindowSpecification::AspectRatio;

    WindowInfo();
    WindowInfo(Window const& window, WindowSpecification const& params);
    ~WindowInfo();
    explicit WindowInfo(WindowInfo const& that);
    WindowInfo& operator=(WindowInfo const& that);

    bool can_be_active() const;

    bool can_morph_to(MirWindowType new_type) const;

    bool must_have_parent() const;

    bool must_not_have_parent() const;

    bool is_visible() const;

    static bool needs_titlebar(MirWindowType type);

    void constrain_resize(mir::geometry::Point& requested_pos, mir::geometry::Size& requested_size) const;

    auto window() const -> Window&;

    auto name() const -> std::string;
    void name(std::string const& name);

    auto type() const -> MirWindowType;
    void type(MirWindowType type);

    auto state() const -> MirWindowState;
    void state(MirWindowState state);

    auto restore_rect() const -> mir::geometry::Rectangle;
    void restore_rect(mir::geometry::Rectangle const& restore_rect);

    auto parent() const -> Window;
    void parent(Window const& parent);

    auto children() const -> std::vector <Window> const&;
    void add_child(Window const& child);
    void remove_child(Window const& child);

    auto min_width() const -> mir::geometry::Width;
    void min_width(mir::geometry::Width min_width);

    auto min_height() const -> mir::geometry::Height;
    void min_height(mir::geometry::Height min_height);

    auto max_width() const -> mir::geometry::Width;
    void max_width(mir::geometry::Width max_width);

    auto max_height() const -> mir::geometry::Height;
    void max_height(mir::geometry::Height max_height);

    auto width_inc() const -> mir::geometry::DeltaX;
    void width_inc(mir::geometry::DeltaX width_inc);

    auto height_inc() const -> mir::geometry::DeltaY;
    void height_inc(mir::geometry::DeltaY height_inc);

    auto min_aspect() const -> AspectRatio;
    void min_aspect(AspectRatio min_aspect);

    auto max_aspect() const -> AspectRatio;
    void max_aspect(AspectRatio max_aspect);

    bool has_output_id() const;
    auto output_id() const -> int;
    void output_id(mir::optional_value<int> output_id);

    auto preferred_orientation() const -> MirOrientationMode;
    void preferred_orientation(MirOrientationMode preferred_orientation);

    auto confine_pointer() const -> MirPointerConfinementState;
    void confine_pointer(MirPointerConfinementState confinement);

    auto shell_chrome() const -> MirShellChrome;
    void shell_chrome(MirShellChrome chrome);

    /// This can be used by client code to store window manager specific information
    auto userdata() const -> std::shared_ptr<void>;
    void userdata(std::shared_ptr<void> userdata);

    void swap(WindowInfo& rhs) { std::swap(self, rhs.self); }

    auto depth_layer() const -> MirDepthLayer;
    void depth_layer(MirDepthLayer depth_layer);

    /// Get the edges of the output that the window is attached to
    /// (only meaningful for windows in state mir_window_state_attached)
    auto attached_edges() const -> MirPlacementGravity;
    /// Set the edges of the output that the window should be attached to
    /// (only meaningful for windows in state mir_window_state_attached)
    void attached_edges(MirPlacementGravity edges);

    /// Mir will try to avoid occluding the area covered by this rectangle (relative to the window)
    /// (only meaningful when the window is attached to an edge)
    auto exclusive_rect() const -> mir::optional_value<mir::geometry::Rectangle>;
    /// Set the area to keep exclusive to this window
    /// (only meaningful when the window is attached to an edge)
    void exclusive_rect(mir::optional_value<mir::geometry::Rectangle> const& rect);

    /// Mir will not render anything outside this rectangle
    auto clip_area() const -> mir::optional_value<mir::geometry::Rectangle>;
    void clip_area(mir::optional_value<mir::geometry::Rectangle> const& area);

    /// The D-bus service name and basename of the app's .desktop file
    /// See http://standards.freedesktop.org/desktop-entry-spec/
    ///@{
    auto application_id() const -> std::string;
    void application_id(std::string const& application_id);
    ///@}

private:
    struct Self;
    std::unique_ptr<Self> self;
};
}

namespace std
{
template<> inline void swap(miral::WindowInfo& lhs, miral::WindowInfo& rhs) { lhs.swap(rhs); }
}

#endif //MIRAL_WINDOW_INFO_H
