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

#include "mir/test/doubles/stub_session.h"

namespace mtd = mir::test::doubles;
namespace ms = mir::scene;

mtd::StubSession::StubSession(pid_t pid)
    : pid(pid)
{}

std::string mtd::StubSession::name() const
{
    return {};
}

pid_t mtd::StubSession::process_id() const
{
    return pid;
}

mir::Fd mtd::StubSession::socket_fd() const
{
    return {};
}

std::shared_ptr<mir::scene::Surface> mtd::StubSession::default_surface() const
{
    return {};
}

void mtd::StubSession::send_error(
    mir::ClientVisibleError const& /*error*/)
{
}

void mtd::StubSession::hide()
{
}

void mtd::StubSession::show()
{
}

void mtd::StubSession::start_prompt_session()
{
}

void mtd::StubSession::stop_prompt_session()
{
}

void mtd::StubSession::suspend_prompt_session()
{
}

void mtd::StubSession::resume_prompt_session()
{
}

auto mtd::StubSession::create_surface(
    std::shared_ptr<Session> const& /*session*/,
    wayland::Weak<frontend::WlSurface> const& /*wayland_surface*/,
    mir::shell::SurfaceSpecification const& /*params*/,
    std::shared_ptr<scene::SurfaceObserver> const& /*observer*/,
    Executor* /*observer_executor*/) -> std::shared_ptr<ms::Surface>
{
    return nullptr;
}

void mtd::StubSession::destroy_surface(std::shared_ptr<scene::Surface> const& /*surface*/)
{
}

auto mtd::StubSession::surface_after(
    std::shared_ptr<mir::scene::Surface> const& /*ptr*/) const -> std::shared_ptr<mir::scene::Surface>
{
    return {};
}

auto mtd::StubSession::create_buffer_stream(
    mir::graphics::BufferProperties const& /*props*/) -> std::shared_ptr<compositor::BufferStream>
{
    return {};
}

void mtd::StubSession::destroy_buffer_stream(std::shared_ptr<frontend::BufferStream> const& /*stream*/)
{
}

void mtd::StubSession::configure_streams(
    mir::scene::Surface& /*surface*/,
    std::vector<mir::shell::StreamSpecification> const& /*config*/)
{
}

namespace
{
// Ensure we don't accidentally have an abstract class
mtd::StubSession instantiation_test [[maybe_unused]];
}
