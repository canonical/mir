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

    void render_titlebar(miral::Buffer titlebar_pixels, mir::geometry::Size scaled_titlebar_size);

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

        struct Widget
        {
            Widget(MirResizeEdge resize_edge) :
                resize_edge{resize_edge}
            {
            }

            mir::geometry::Rectangle rect;
            ButtonState state{ButtonState::Up};
            // mir_resize_edge_none is used to mean the widget moves the window
            std::optional<MirResizeEdge> const resize_edge;
        };

        auto widget_at(mir::geometry::Point location) -> std::optional<std::shared_ptr<Widget>>;

        void process_drag(miral::DeviceEvent& device, miral::InputContext ctx);
        void process_enter(miral::DeviceEvent& device, miral::InputContext ctx);
        void process_leave(miral::InputContext ctx);
        void process_down();
        void process_up();

        void widget_drag(Widget& widget, miral::InputContext ctx);
        void widget_leave(Widget& widget, miral::InputContext ctx);
        void widget_enter(Widget& widget, miral::InputContext ctx);
        void widget_down(Widget& widget);
        void widget_up(Widget& widget);

        void update_window_state(WindowState const& window_state);
        auto resize_edge_rect(WindowState const& window_state, MirResizeEdge resize_edge) -> mir::geometry::Rectangle;

        std::vector<std::shared_ptr<Widget>> widgets = {
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

    InputManager input_manager;
};
