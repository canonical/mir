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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SCENE_APPLICATION_SESSION_H_
#define MIR_SCENE_APPLICATION_SESSION_H_

#include "mir/scene/session.h"

#include "mir/observer_registrar.h"

#include <atomic>
#include <set>
#include <map>
#include <mutex>

namespace mir
{
namespace frontend
{
class ClientBuffers;
}
namespace graphics
{
class GraphicBufferAllocator;
class BufferAttribute;
}
namespace shell { class SurfaceStack; }
namespace scene
{
class SessionListener;
class Surface;
class SnapshotStrategy;
class BufferStreamFactory;
class SurfaceFactory;

class ApplicationSession : public Session
{
public:
    ApplicationSession(
        std::shared_ptr<shell::SurfaceStack> const& surface_stack,
        std::shared_ptr<SurfaceFactory> const& surface_factory,
        std::shared_ptr<BufferStreamFactory> const& buffer_stream_factory,
        pid_t pid,
        std::string const& session_name,
        std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
        std::shared_ptr<SessionListener> const& session_listener,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    ~ApplicationSession();

    auto create_surface(
        std::shared_ptr<Session> const& session,
        SurfaceCreationParameters const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer) -> std::shared_ptr<Surface> override;
    void destroy_surface(std::shared_ptr<Surface> const& surface) override;
    auto surface_after(std::shared_ptr<Surface> const& sruface) const -> std::shared_ptr<Surface> override;

    void take_snapshot(SnapshotCallback const& snapshot_taken) override;
    std::shared_ptr<Surface> default_surface() const override;

    std::string name() const override;
    pid_t process_id() const override;

    void hide() override;
    void show() override;

    void send_error(ClientVisibleError const& error) override;
    void send_input_config(MirInputConfig const& devices) override;

    void set_lifecycle_state(MirLifecycleState state) override;

    void start_prompt_session() override;
    void stop_prompt_session() override;
    void suspend_prompt_session() override;
    void resume_prompt_session() override;

    auto create_buffer_stream(graphics::BufferProperties const& params)
        -> std::shared_ptr<compositor::BufferStream> override;
    void destroy_buffer_stream(std::shared_ptr<frontend::BufferStream> const& stream) override;
    void configure_streams(Surface& surface, std::vector<shell::StreamSpecification> const& config) override;

    /// Returns if the application session knows about the given buffer stream
    auto has_buffer_stream(std::shared_ptr<compositor::BufferStream> const& stream) -> bool;

protected:
    ApplicationSession(ApplicationSession const&) = delete;
    ApplicationSession& operator=(ApplicationSession const&) = delete;

private:
    std::shared_ptr<shell::SurfaceStack> const surface_stack;
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::shared_ptr<BufferStreamFactory> const buffer_stream_factory;
    pid_t const pid;
    std::string const session_name;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<frontend::EventSink> const event_sink;
    std::shared_ptr<graphics::GraphicBufferAllocator> const gralloc;

    std::vector<std::shared_ptr<Surface>> surfaces;
    std::set<std::shared_ptr<compositor::BufferStream>> streams;
    std::map<
        std::weak_ptr<Surface>,
        std::weak_ptr<compositor::BufferStream>,
        std::owner_less<std::weak_ptr<Surface>>> default_content_map;
    std::mutex mutable surfaces_and_streams_mutex;
};

}
} // namespace mir

#endif // MIR_SCENE_APPLICATION_SESSION_H_
