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

#ifndef MIR_TEST_DOUBLES_MOCK_SCENE_SESSION_H_
#define MIR_TEST_DOUBLES_MOCK_SCENE_SESSION_H_

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/graphics/display_configuration.h"
#include "mir/input/mir_input_config.h"
#include "mir/client_visible_error.h"
#include "mir/shell/surface_specification.h"
#include "mir/wayland/weak.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSceneSession : public scene::Session
{
    MOCK_METHOD(std::shared_ptr<scene::Surface>, create_surface, (
        std::shared_ptr<Session> const&,
        wayland::Weak<frontend::WlSurface> const&,
        shell::SurfaceSpecification const&,
        std::shared_ptr<scene::SurfaceObserver> const&,
        Executor*), (override));
    MOCK_METHOD(void, destroy_surface, (std::shared_ptr<scene::Surface> const&), (override));
    MOCK_METHOD(std::shared_ptr<scene::Surface>, surface_after, (
        std::shared_ptr<scene::Surface> const&), (const override));

    MOCK_METHOD(std::shared_ptr<scene::Surface>, default_surface, (), (const override));

    MOCK_METHOD(std::string, name, (), (const override));
    MOCK_METHOD(pid_t, process_id, (), (const override));
    MOCK_METHOD(Fd, socket_fd, (), (const override));

    MOCK_METHOD(void, hide, (), (override));
    MOCK_METHOD(void, show, (), (override));

    MOCK_METHOD(void, send_error, (ClientVisibleError const&), (override));

    MOCK_METHOD(void, start_prompt_session, (), (override));
    MOCK_METHOD(void, stop_prompt_session, (), (override));
    MOCK_METHOD(void, suspend_prompt_session, (), (override));
    MOCK_METHOD(void, resume_prompt_session, (), (override));

    MOCK_METHOD(std::shared_ptr<compositor::BufferStream>, create_buffer_stream, (
        graphics::BufferProperties const&), (override));
    MOCK_METHOD(void, destroy_buffer_stream, (std::shared_ptr<frontend::BufferStream> const&), (override));
    
    MOCK_METHOD(void, configure_streams, (scene::Surface&, std::vector<shell::StreamSpecification> const&), (override));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_H_
