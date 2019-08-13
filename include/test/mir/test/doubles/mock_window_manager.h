/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_
#define MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_

#include "mir/shell/window_manager.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface_creation_parameters.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockWindowManager : shell::WindowManager
{
    MockWindowManager()
    {
        using namespace ::testing;
        ON_CALL(*this, add_surface(_,_,_)).WillByDefault(Invoke(add_surface_default));
    }

    MOCK_METHOD1(add_session, void (std::shared_ptr<scene::Session> const&));
    MOCK_METHOD1(remove_session, void (std::shared_ptr<scene::Session> const&));

    MOCK_METHOD3(add_surface, std::shared_ptr<scene::Surface>(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<std::shared_ptr<scene::Surface>(
            std::shared_ptr<scene::Session> const& session,
            scene::SurfaceCreationParameters const& params)> const& build));

    MOCK_METHOD3(modify_surface, void(std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, shell::SurfaceSpecification const&));
    MOCK_METHOD2(remove_surface, void(std::shared_ptr<scene::Session> const&, std::weak_ptr<scene::Surface> const&));

    MOCK_METHOD1(add_display, void(geometry::Rectangle const&));
    MOCK_METHOD1(remove_display, void(geometry::Rectangle const&));

    MOCK_METHOD1(handle_keyboard_event, bool(MirKeyboardEvent const*));
    MOCK_METHOD1(handle_touch_event, bool(MirTouchEvent const*));
    MOCK_METHOD1(handle_pointer_event, bool(MirPointerEvent const*));

    MOCK_METHOD3(handle_raise_surface, void(std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, uint64_t));
    MOCK_METHOD3(handle_request_drag_and_drop, void(std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, uint64_t));
    MOCK_METHOD3(handle_request_move, void(std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, uint64_t));
    MOCK_METHOD4(handle_request_resize, void(std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, uint64_t, MirResizeEdge));

    MOCK_METHOD4(set_surface_attribute,
        int(std::shared_ptr<scene::Session> const& session,
            std::shared_ptr<scene::Surface> const& surface,
            MirWindowAttrib attrib,
            int value));

    static auto add_surface_default(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<std::shared_ptr<scene::Surface>(
            std::shared_ptr<scene::Session> const& session,
            scene::SurfaceCreationParameters const& params)> const& build) -> std::shared_ptr<scene::Surface>
        { return build(session, params); }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_ */
