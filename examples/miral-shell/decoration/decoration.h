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

#include "miral/decoration.h"
#include "miral/decoration_adapter.h"

#include "mir_toolkit/common.h"
#include "mir/geometry/forward.h"
#include "mir/geometry/rectangle.h"

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
namespace decoration
{
class WindowState;
}
}

// Placeholder names
class UserDecoration : public miral::Decoration
{
public:
    UserDecoration();

    using Renderer = miral::decoration::Renderer;
    void render_titlebar(Renderer::Buffer const& titlebar_buffer);

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

        using InputContext = miral::decoration::InputContext;
        void process_drag(miral::DeviceEvent& device, InputContext ctx);
        void process_enter(miral::DeviceEvent& device, InputContext ctx);
        void process_leave(InputContext ctx);
        void process_down();
        void process_up(InputContext ctx);
        void process_move(miral::DeviceEvent& device, InputContext ctx);

        void widget_drag(Widget& widget, InputContext ctx);
        void widget_leave(Widget& widget, InputContext ctx);
        void widget_enter(Widget& widget, InputContext ctx);
        void widget_down(Widget& widget);
        void widget_up(Widget& widget, InputContext ctx);

        void update_window_state(miral::decoration::WindowState const& window_state);

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

    void scale_changed(float new_scale);

    InputManager input_manager;
    static Renderer::Pixel const focused_titlebar_color = 0xFF323232;
    static Renderer::Pixel const unfocused_titlebar_color = 0xFF525252;
    Renderer::Pixel current_titlebar_color = focused_titlebar_color;
    float scale = 1.0;
};
