/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/test/doubles/stub_session.h"

namespace mtd = mir::test::doubles;

std::shared_ptr<mir::frontend::Surface> mtd::StubSession::get_surface(
    mir::frontend::SurfaceId /*surface*/) const
{
    return {};
}

std::string mtd::StubSession::name() const
{
    return {};
}

void mtd::StubSession::force_requests_to_complete()
{
}

pid_t mtd::StubSession::process_id() const
{
    return {0};
}

void mtd::StubSession::take_snapshot(
    mir::scene::SnapshotCallback const& /*snapshot_taken*/)
{
}

std::shared_ptr<mir::scene::Surface> mtd::StubSession::default_surface() const
{
    return {};
}

void mtd::StubSession::set_lifecycle_state(MirLifecycleState /*state*/)
{
}

void mtd::StubSession::send_display_config(
    mir::graphics::DisplayConfiguration const& /*configuration*/)
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

mir::frontend::SurfaceId mtd::StubSession::create_surface(
    mir::scene::SurfaceCreationParameters const& /*params*/,
    std::shared_ptr<frontend::EventSink> const& /*sink*/)
{
    return mir::frontend::SurfaceId{0};
}

void mtd::StubSession::destroy_surface(mir::frontend::SurfaceId /*surface*/)
{
}

std::shared_ptr<mir::scene::Surface> mtd::StubSession::surface(
    mir::frontend::SurfaceId /*surface*/) const
{
    return {};
}

std::shared_ptr<mir::scene::Surface> mtd::StubSession::surface_after(
    std::shared_ptr<mir::scene::Surface> const& /*ptr*/) const
{
    return {};
}

std::shared_ptr<mir::frontend::BufferStream> mtd::StubSession::get_buffer_stream(
    mir::frontend::BufferStreamId /*stream*/) const
{
    return {};
}

mir::frontend::BufferStreamId mtd::StubSession::create_buffer_stream(
    mir::graphics::BufferProperties const& /*props*/)
{
    return {};
}

void mtd::StubSession::destroy_buffer_stream(mir::frontend::BufferStreamId /*stream*/)
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
mtd::StubSession instantiation_test;
}
