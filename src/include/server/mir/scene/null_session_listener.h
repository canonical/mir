/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SCENE_NULL_SESSION_LISTENER_H_
#define MIR_SCENE_NULL_SESSION_LISTENER_H_

#include "mir/scene/session_listener.h"

namespace mir
{
namespace scene
{
class NullSessionListener : public SessionListener
{
public:
    NullSessionListener() = default;
    virtual ~NullSessionListener() noexcept(true) = default;

    void starting(std::shared_ptr<Session> const&) override {}
    void stopping(std::shared_ptr<Session> const&) override {}
    void focused(std::shared_ptr<Session> const&) override {}
    void unfocused() override {}

    void surface_created(Session&, std::shared_ptr<Surface> const&) override {}
    void destroying_surface(Session&, std::shared_ptr<Surface> const&) override {}

    void buffer_stream_created(Session&, std::shared_ptr<frontend::BufferStream> const&) override {}
    void buffer_stream_destroyed(Session&, std::shared_ptr<frontend::BufferStream> const&) override {}
protected:
    NullSessionListener(const NullSessionListener&) = delete;
    NullSessionListener& operator=(const NullSessionListener&) = delete;
};

}
}

#endif // MIR_SCENE_NULL_SESSION_LISTENER_H_
