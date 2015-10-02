/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.   If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SCENE_SESSION_H_
#define MIR_SCENE_SESSION_H_

#include "mir/frontend/session.h"
#include "mir/scene/snapshot.h"

#include <vector>
#include <sys/types.h>

namespace mir
{
namespace frontend { class EventSink; }
namespace shell { struct StreamSpecification; }
namespace scene
{
class Surface;
struct SurfaceCreationParameters;

class Session : public frontend::Session
{
public:
    virtual void force_requests_to_complete() = 0;
    virtual pid_t process_id() const = 0;

    virtual void take_snapshot(SnapshotCallback const& snapshot_taken) = 0;
    virtual std::shared_ptr<Surface> default_surface() const = 0;
    virtual void set_lifecycle_state(MirLifecycleState state) = 0;
    virtual void send_display_config(graphics::DisplayConfiguration const&) = 0;

    virtual void hide() = 0;
    virtual void show() = 0;

    virtual void start_prompt_session() = 0;
    virtual void stop_prompt_session() = 0;
    virtual void suspend_prompt_session() = 0;
    virtual void resume_prompt_session() = 0;

    virtual frontend::SurfaceId create_surface(
        SurfaceCreationParameters const& params,
        std::shared_ptr<frontend::EventSink> const& sink) = 0;
    virtual void destroy_surface(frontend::SurfaceId surface) = 0;

    virtual std::shared_ptr<Surface> surface(frontend::SurfaceId surface) const = 0;
    virtual std::shared_ptr<Surface> surface_after(std::shared_ptr<Surface> const&) const = 0;

    virtual std::shared_ptr<frontend::BufferStream> get_buffer_stream(frontend::BufferStreamId stream) const = 0;

    virtual frontend::BufferStreamId create_buffer_stream(graphics::BufferProperties const& props) = 0;
    virtual void destroy_buffer_stream(frontend::BufferStreamId stream) = 0;
    virtual void configure_streams(Surface& surface, std::vector<shell::StreamSpecification> const& config) = 0;
};
}
}

#endif // MIR_SCENE_SESSION_H_
