/*
 * Copyright © 2016-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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

#ifndef MIR_CLIENT_WINDOW_SPEC_H
#define MIR_CLIENT_WINDOW_SPEC_H

#include <mir/client/window.h>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_window.h>
#include <mir_toolkit/version.h>
#include <mir_toolkit/rs/mir_render_surface.h>

#include <memory>

namespace mir
{
namespace client
{
/// Handle class for MirWindowSpec - provides automatic reference counting, method chaining.
class WindowSpec
{
public:
    explicit WindowSpec(MirWindowSpec* spec) : self{spec, deleter} {}

    static auto for_normal_window(MirConnection* connection, int width, int height) -> WindowSpec
    {
        return WindowSpec{mir_create_normal_window_spec(connection, width, height)};
    }

    static auto for_menu(MirConnection* connection,
                         int width,
                         int height,
                         MirWindow* parent,
                         MirRectangle* rect,
                         MirEdgeAttachment edge) -> WindowSpec
    {
        auto spec = WindowSpec{mir_create_menu_window_spec(connection, width, height, parent, rect, edge)};
        return spec;
    }

    static auto for_tip(MirConnection* connection,
                        int width,
                        int height,
                        MirWindow* parent,
                        MirRectangle* rect,
                        MirEdgeAttachment edge) -> WindowSpec
    {
        auto spec = WindowSpec{mir_create_tip_window_spec(connection, width, height, parent, rect, edge)};
        return spec;
    }

    static auto for_dialog(MirConnection* connection,
                           int width,
                           int height)-> WindowSpec
    {
        auto spec = WindowSpec{mir_create_dialog_window_spec(connection, width, height)};
        return spec;
    }

    static auto for_dialog(MirConnection* connection,
                           int width,
                           int height,
                           MirWindow* parent) -> WindowSpec
    {
        return for_dialog(connection, width, height).set_parent(parent);
    }

    static auto for_input_method(MirConnection* connection, int width, int height, MirWindow* parent)
    {
        auto spec = WindowSpec{mir_create_input_method_window_spec(connection, width, height)}
            .set_parent(parent);
        return spec;
    }

    static auto for_satellite(MirConnection* connection, int width, int height, MirWindow* parent)
    {
#if MIR_CLIENT_API_VERSION >= MIR_VERSION_NUMBER(0, 27, 0)
        return WindowSpec{mir_create_satellite_window_spec(connection, width, height, parent)};
#else
        // There's no mir_create_satellite_window_spec()
        auto spec = WindowSpec{mir_create_window_spec(connection)}
            .set_size(width, height)
            .set_type(mir_window_type_satellite)
            .set_parent(parent);

        mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware); // Required protobuf field for create_window()
        mir_window_spec_set_pixel_format(spec, mir_pixel_format_invalid);  // Required protobuf field for create_window()
        return spec;
#endif
    }

    static auto for_gloss(MirConnection* connection, int width, int height)
    {
#if MIR_CLIENT_API_VERSION >= MIR_VERSION_NUMBER(0, 27, 0)
        return WindowSpec{mir_create_gloss_window_spec(connection, width, height)};
#else
        // There's no mir_create_gloss_window_spec()
        auto spec = WindowSpec{mir_create_window_spec(connection)}
            .set_size(width, height)
            .set_type(mir_window_type_gloss);

        mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware); // Required protobuf field for create_window()
        mir_window_spec_set_pixel_format(spec, mir_pixel_format_invalid);  // Required protobuf field for create_window()
        return spec;
#endif
    }

    static auto for_changes(MirConnection* connection) -> WindowSpec
    {
        return WindowSpec{mir_create_window_spec(connection)};
    }

    auto set_type(MirWindowType type) -> WindowSpec&
    {
        mir_window_spec_set_type(*this, type);
        return *this;
    }

    auto set_shell_chrome(MirShellChrome chrome) -> WindowSpec&
    {
        mir_window_spec_set_shell_chrome(*this, chrome);
        return *this;
    }

    auto set_min_size(int min_width, int min_height) -> WindowSpec&
    {
        mir_window_spec_set_min_width(*this, min_width);
        mir_window_spec_set_min_height(*this, min_height);
        return *this;
    }

    auto set_max_size(int max_width, int max_height) -> WindowSpec&
    {
        mir_window_spec_set_max_width(*this, max_width);
        mir_window_spec_set_max_height(*this, max_height);
        return *this;
    }

    auto set_size_inc(int width_inc, int height_inc) -> WindowSpec&
    {
        mir_window_spec_set_width_increment(*this, width_inc);
        mir_window_spec_set_height_increment(*this, height_inc);
        return *this;
    }

    auto set_size(int width, int height) -> WindowSpec&
    {
        mir_window_spec_set_width(*this, width);
        mir_window_spec_set_height(*this, height);
        return *this;
    }

    auto set_name(char const* name) -> WindowSpec&
    {
        mir_window_spec_set_name(*this, name);
        return *this;
    }

    auto set_event_handler(MirWindowEventCallback callback, void* context) -> WindowSpec&
    {
        mir_window_spec_set_event_handler(*this, callback, context);
        return *this;
    }

    auto set_fullscreen_on_output(uint32_t output_id) -> WindowSpec&
    {
        mir_window_spec_set_fullscreen_on_output(*this, output_id);
        return *this;
    }

    auto set_placement(const MirRectangle* rect,
                       MirPlacementGravity rect_gravity,
                       MirPlacementGravity surface_gravity,
                       MirPlacementHints   placement_hints,
                       int                 offset_dx,
                       int                 offset_dy) -> WindowSpec&
    {
        mir_window_spec_set_placement(*this, rect, rect_gravity, surface_gravity, placement_hints, offset_dx, offset_dy);
        return *this;
    }

    auto set_parent(MirWindow* parent) -> WindowSpec&
    {
        mir_window_spec_set_parent(*this, parent);
        return *this;
    }

    auto set_state(MirWindowState state) -> WindowSpec&
    {
        mir_window_spec_set_state(*this, state);
        return *this;
    }

    auto add_surface(MirRenderSurface* surface, int width, int height, int displacement_x, int displacement_y)
    -> WindowSpec&
    {
#if MIR_CLIENT_API_VERSION < MIR_VERSION_NUMBER(0, 27, 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        mir_window_spec_add_render_surface(*this, surface, width, height, displacement_x, displacement_y);
#if MIR_CLIENT_API_VERSION < MIR_VERSION_NUMBER(0, 27, 0)
#pragma GCC diagnostic pop
#endif
        return *this;
    }

    template<typename Context>
    void create_window(void (* callback)(MirWindow*, Context*), Context* context) const
    {
        mir_create_window(*this, reinterpret_cast<MirWindowCallback>(callback), context);
    }

    auto create_window() const -> Window
    {
        return Window{mir_create_window_sync(*this)};
    }

    void apply_to(MirWindow* window) const
    {
        mir_window_apply_spec(window, *this);
    }

    operator MirWindowSpec*() const { return self.get(); }

private:
    static void deleter(MirWindowSpec* spec) { mir_window_spec_release(spec); }
    std::shared_ptr<MirWindowSpec> self;
};

// Provide a deleted overload to avoid double release "accidents".
void mir_window_spec_release(WindowSpec const& spec) = delete;
void mir_surface_spec_release(WindowSpec const& spec) = delete;
}
}

#endif //MIRAL_TOOLKIT_WINDOW_SPEC_H_H
