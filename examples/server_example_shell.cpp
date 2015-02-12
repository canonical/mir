/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_shell.h"
#include "server_example_tiling_window_manager.h"
#include "server_example_basic_window_manager.h"

#include "mir/abnormal_exit.h"
#include "mir/server.h"
#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"
#include "mir/shell/display_layout.h"

namespace me = mir::examples;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

///\example server_example_shell.cpp
/// Demonstrate a shell supporting selection of a window manager

char const* const me::wm_option = "window-manager";
char const* const me::wm_description = "window management strategy [{tiling|fullscreen}]";

namespace
{
char const* const wm_tiling = "tiling";
char const* const wm_fullscreen = "fullscreen";

struct NullSessionInfo
{
};

struct NullSurfaceInfo
{
    NullSurfaceInfo(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<ms::Surface> const& /*surface*/) {}
};


// Very simple - make every surface fullscreen
class FullscreenWindowManagerPolicy
{
public:
    using Tools = me::BasicWindowManagerTools<NullSessionInfo, NullSurfaceInfo>;
    using SessionInfoMap = typename me::SessionTo<NullSessionInfo>::type;

    FullscreenWindowManagerPolicy(Tools* const /*tools*/, std::shared_ptr<msh::DisplayLayout> const& display_layout) :
        display_layout{display_layout} {}

    void handle_session_info_updated(SessionInfoMap& /*session_info*/, Rectangles const& /*displays*/) {}

    void handle_displays_updated(SessionInfoMap& /*session_info*/, Rectangles const& /*displays*/) {}

    auto handle_place_new_surface(
        std::shared_ptr<ms::Session> const& /*session*/,
        ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters
    {
        auto placed_parameters = request_parameters;

        Rectangle rect{request_parameters.top_left, request_parameters.size};
        display_layout->size_to_output(rect);
        placed_parameters.size = rect.size;

        return placed_parameters;
    }

    void handle_new_surface(std::shared_ptr<ms::Session> const& /*session*/, std::shared_ptr<ms::Surface> const& /*surface*/)
    {
    }

    void handle_delete_surface(std::shared_ptr<ms::Session> const& /*session*/, std::weak_ptr<ms::Surface> const& /*surface*/) {}

    int handle_set_state(std::shared_ptr<ms::Surface> const& /*surface*/, MirSurfaceState value)
        { return value; }

    bool handle_key_event(MirKeyInputEvent const* /*event*/) { return false; }

    bool handle_touch_event(MirTouchInputEvent const* /*event*/) { return false; }

    bool handle_pointer_event(MirPointerInputEvent const* /*event*/) { return false; }

private:
    std::shared_ptr<msh::DisplayLayout> const display_layout;
};

}

using TilingWindowManager = me::BasicWindowManager<me::TilingWindowManagerPolicy, me::TilingSessionInfo, me::TilingSurfaceInfo>;
using FullscreenWindowManager = me::BasicWindowManager<FullscreenWindowManagerPolicy, NullSessionInfo, NullSurfaceInfo>;

auto me::ShellFactory::shell() -> std::shared_ptr<me::Shell>
{
    auto tmp = shell_.lock();

    if (!tmp)
    {
        auto const options = server.get_options();
        auto const selection = options->get<std::string>(wm_option);

        std::function<std::shared_ptr<WindowManager>(shell::FocusController* focus_controller)> wm_builder;

        if (selection == wm_tiling)
        {
            wm_builder = [this](msh::FocusController* focus_controller) -> std::shared_ptr<WindowManager>
                {
                    return std::make_shared<TilingWindowManager>(focus_controller);
                };
        }
        else if (selection == wm_fullscreen)
        {
            wm_builder = [this](msh::FocusController* focus_controller) -> std::shared_ptr<WindowManager>
                {
                    return std::make_shared<FullscreenWindowManager>(focus_controller, server.the_shell_display_layout());
                };
        }
        else
            throw mir::AbnormalExit("Unknown window manager: " + selection);


        tmp = std::make_shared<GenericShell>(
            server.the_input_targeter(),
            server.the_surface_coordinator(),
            server.the_session_coordinator(),
            server.the_prompt_session_manager(),
            wm_builder);

        server.the_composite_event_filter()->prepend(tmp);
        shell_ = tmp;
    }

    return tmp;
}
