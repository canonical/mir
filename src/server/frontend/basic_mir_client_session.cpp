/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "mir/frontend/basic_mir_client_session.h"

#include "mir/shell/shell.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;

mf::BasicMirClientSession::BasicMirClientSession(std::shared_ptr<scene::Session> session)
    : session_{session}
{
}

auto mf::BasicMirClientSession::session() const -> std::shared_ptr<scene::Session>
{
    return session_;
}

auto mf::BasicMirClientSession::name() const -> std::string
{
    return session_->name();
}

auto mf::BasicMirClientSession::get_surface(SurfaceId surface) const -> std::shared_ptr<Surface>
{
    return session_->surface(surface);
}

auto mf::BasicMirClientSession::create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId
{
    return session_->create_buffer_stream(props);
}

auto mf::BasicMirClientSession::get_buffer_stream(BufferStreamId stream) const -> std::shared_ptr<BufferStream>
{
    return session_->get_buffer_stream(stream);
}

void mf::BasicMirClientSession::destroy_buffer_stream(BufferStreamId stream)
{
    session_->destroy_buffer_stream(stream);
}
