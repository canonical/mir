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

#ifndef MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_
#define MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_

#include "mir/shell/window_manager.h"
#include "mir/shell/surface_specification.h"

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

    MOCK_METHOD(void, add_session, (std::shared_ptr<scene::Session> const&), (override));
    MOCK_METHOD(void, remove_session, (std::shared_ptr<scene::Session> const&), (override));

    MOCK_METHOD(std::shared_ptr<scene::Surface>, add_surface, (
        std::shared_ptr<scene::Session> const& session,
        shell::SurfaceSpecification const& params,
        std::function<std::shared_ptr<scene::Surface>(
            std::shared_ptr<scene::Session> const& session,
            shell::SurfaceSpecification const& params)> const& build), (override));

    MOCK_METHOD(void, surface_ready, (std::shared_ptr<scene::Surface> const&), (override));
    MOCK_METHOD(void, modify_surface, (std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, shell::SurfaceSpecification const&), (override));
    MOCK_METHOD(void, remove_surface, (std::shared_ptr<scene::Session> const&, std::weak_ptr<scene::Surface> const&), (override));

    MOCK_METHOD(void, add_display, (geometry::Rectangle const&), (override));
    MOCK_METHOD(void, remove_display, (geometry::Rectangle const&), (override));

    MOCK_METHOD(bool, handle_keyboard_event, (MirKeyboardEvent const*), (override));
    MOCK_METHOD(bool, handle_touch_event, (MirTouchEvent const*), (override));
    MOCK_METHOD(bool, handle_pointer_event, (MirPointerEvent const*), (override));

    MOCK_METHOD(void, handle_raise_surface, (std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, uint64_t), (override));
    MOCK_METHOD(void, handle_request_move, (std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, MirInputEvent const*), (override));
    MOCK_METHOD(void, handle_request_resize, (std::shared_ptr<scene::Session> const&, std::shared_ptr<scene::Surface> const&, MirInputEvent const*, MirResizeEdge), (override));

    MOCK_METHOD(int, set_surface_attribute, (std::shared_ptr<scene::Session> const& session,
            std::shared_ptr<scene::Surface> const& surface,
            MirWindowAttrib attrib,
            int value), (override));

    static auto add_surface_default(
        std::shared_ptr<scene::Session> const& session,
        shell::SurfaceSpecification const& params,
        std::function<std::shared_ptr<scene::Surface>(
            std::shared_ptr<scene::Session> const& session,
            shell::SurfaceSpecification const& params)> const& build) -> std::shared_ptr<scene::Surface>
        { return build(session, params); }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_WINDOW_MANAGER_H_ */
