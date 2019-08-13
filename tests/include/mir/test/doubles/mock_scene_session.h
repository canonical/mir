/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SCENE_SESSION_H_
#define MIR_TEST_DOUBLES_MOCK_SCENE_SESSION_H_

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/graphics/display_configuration.h"
#include "mir/input/mir_input_config.h"
#include "mir/client_visible_error.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSceneSession : public scene::Session
{
    MOCK_METHOD2(create_surface,
        std::shared_ptr<scene::Surface>(
            scene::SurfaceCreationParameters const&,
            std::shared_ptr<frontend::EventSink> const&));
    MOCK_METHOD1(destroy_surface, void(frontend::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<frontend::Surface>(frontend::SurfaceId));
    MOCK_CONST_METHOD1(surface, std::shared_ptr<scene::Surface>(frontend::SurfaceId));
    MOCK_CONST_METHOD1(surface_id, frontend::SurfaceId(std::shared_ptr<scene::Surface> const&));
    MOCK_CONST_METHOD1(surface_after, std::shared_ptr<scene::Surface>(std::shared_ptr<scene::Surface> const&));

    MOCK_METHOD1(take_snapshot, void(scene::SnapshotCallback const&));
    MOCK_CONST_METHOD0(default_surface, std::shared_ptr<scene::Surface>());

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD0(process_id, pid_t());

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());

    MOCK_METHOD1(send_error, void(ClientVisibleError const&));
    MOCK_METHOD1(send_input_config, void(MirInputConfig const&));
    MOCK_METHOD3(configure_surface, int(frontend::SurfaceId, MirWindowAttrib, int));

    MOCK_METHOD1(set_lifecycle_state, void(MirLifecycleState state));

    MOCK_METHOD0(start_prompt_session, void());
    MOCK_METHOD0(stop_prompt_session, void());
    MOCK_METHOD0(suspend_prompt_session, void());
    MOCK_METHOD0(resume_prompt_session, void());

    MOCK_CONST_METHOD1(get_buffer_stream, std::shared_ptr<frontend::BufferStream>(frontend::BufferStreamId));
    MOCK_METHOD1(destroy_buffer_stream, void(frontend::BufferStreamId));
    MOCK_METHOD1(create_buffer_stream, frontend::BufferStreamId(graphics::BufferProperties const&));
    
    MOCK_METHOD2(configure_streams, void(scene::Surface&, std::vector<shell::StreamSpecification> const&));
    MOCK_METHOD1(destroy_surface, void (std::weak_ptr<scene::Surface> const&));

    MOCK_METHOD1(create_buffer, graphics::BufferID(graphics::BufferProperties const&));
    MOCK_METHOD3(create_buffer, graphics::BufferID(geometry::Size, uint32_t, uint32_t));
    MOCK_METHOD2(create_buffer, graphics::BufferID(geometry::Size, MirPixelFormat));
    MOCK_METHOD1(destroy_buffer, void(graphics::BufferID));
    MOCK_METHOD1(get_buffer, std::shared_ptr<graphics::Buffer>(graphics::BufferID));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_H_
