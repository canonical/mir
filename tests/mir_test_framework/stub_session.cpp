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

#include "mir/test/doubles/stub_session.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir_test_framework/stub_platform_native_buffer.h"

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
    mir::scene::SurfaceCreationParameters const& /*params*/,
    std::shared_ptr<frontend::EventSink> const& /*sink*/) -> std::shared_ptr<ms::Surface>
{
    return nullptr;
}

void mtd::StubSession::destroy_surface(mir::frontend::SurfaceId /*surface*/)
{
}

auto mtd::StubSession::surface(
    frontend::SurfaceId /*surface*/) const -> std::shared_ptr<scene::Surface>
{
    return {};
}

auto mtd::StubSession::surface_id(
    std::shared_ptr<scene::Surface> const& /*surface*/) const -> frontend::SurfaceId
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

void mtd::StubSession::destroy_surface(std::weak_ptr<scene::Surface> const& /*surface*/)
{
}

void mtd::StubSession::send_input_config(MirInputConfig const& /*config*/)
{
}

namespace
{
// Ensure we don't accidentally have an abstract class
mtd::StubSession instantiation_test;
}
