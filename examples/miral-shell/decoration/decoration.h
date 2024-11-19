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

#include "mir_toolkit/common.h"

#include "mir/geometry/forward.h"
#include "mir/geometry/rectangle.h"
#include "miral/decoration.h"
#include "miral/decoration_adapter.h"

#include <memory>
#include <optional>
#include <vector>

namespace mir
{
class Server;
}

namespace miral
{
class DecorationManagerAdapter;
class InputContext;
class DeviceEvent;
}

class WindowState; //  TODO move to miral

// Placeholder names
class UserDecoration : public miral::Decoration
{
public:
    UserDecoration();

    void render_titlebar(miral::Renderer::Buffer const& titlebar_buffer);
    void fill_solid_color(miral::Renderer::Buffer const& left_border_buffer, miral::Pixel color);

    static auto create_manager(mir::Server&)
        -> std::shared_ptr<miral::DecorationManagerAdapter>;

    struct InputManager
    {
        enum class ButtonState
        {
            Up,
            Hovered,
            Down,
        };

        enum class ButtonFunction
        {
            Close,
            Maximize,
            Minimize,
        };

        struct Widget
        {
            Widget(MirResizeEdge resize_edge) :
                resize_edge{resize_edge}
            {
            }

            Widget(ButtonFunction button)
                : button{button}
            {
            }

            mir::geometry::Rectangle rect;
            ButtonState state{ButtonState::Up};
            // mir_resize_edge_none is used to mean the widget moves the window
            std::optional<MirResizeEdge> const resize_edge;
            std::optional<ButtonFunction> const button;
        };

        auto widget_at(mir::geometry::Point location) -> std::optional<std::shared_ptr<Widget>>;

        void process_drag(miral::DeviceEvent& device, miral::InputContext ctx);
        void process_enter(miral::DeviceEvent& device, miral::InputContext ctx);
        void process_leave(miral::InputContext ctx);
        void process_down();
        void process_up(miral::InputContext ctx);
        void process_move(miral::DeviceEvent& device, miral::InputContext ctx);

        void widget_drag(Widget& widget, miral::InputContext ctx);
        void widget_leave(Widget& widget, miral::InputContext ctx);
        void widget_enter(Widget& widget, miral::InputContext ctx);
        void widget_down(Widget& widget);
        void widget_up(Widget& widget, miral::InputContext ctx);

        void update_window_state(WindowState const& window_state);

        auto buttons() const -> std::vector<std::shared_ptr<Widget const>>;

        std::vector<std::shared_ptr<Widget>> widgets = {
            std::make_shared<Widget>(ButtonFunction::Close),
            std::make_shared<Widget>(ButtonFunction::Maximize),
            std::make_shared<Widget>(ButtonFunction::Minimize),
            std::make_shared<Widget>(mir_resize_edge_northwest),
            std::make_shared<Widget>(mir_resize_edge_northeast),
            std::make_shared<Widget>(mir_resize_edge_southwest),
            std::make_shared<Widget>(mir_resize_edge_southeast),
            std::make_shared<Widget>(mir_resize_edge_north),
            std::make_shared<Widget>(mir_resize_edge_south),
            std::make_shared<Widget>(mir_resize_edge_west),
            std::make_shared<Widget>(mir_resize_edge_east),
            std::make_shared<Widget>(mir_resize_edge_none),
        };
        std::optional<std::shared_ptr<Widget>> active_widget;
    };

    void focused();
    void unfocused();

    void minimized();
    void maximized();
    void normal();

    InputManager input_manager;
    static miral::Pixel const focused_titlebar_color = 0xFF323232;
    static miral::Pixel const unfocused_titlebar_color = 0xFF525252;
    miral::Pixel current_titlebar_color = focused_titlebar_color;
};
