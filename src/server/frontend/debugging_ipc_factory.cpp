/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "debugging_ipc_factory.h"

#include "debugging_session_mediator.h"
#include "mir/graphics/graphic_buffer_allocator.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;

std::shared_ptr<mf::detail::DisplayServer> mf::DebuggingIpcFactory::make_mediator(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<mg::Platform> const& graphics_platform,
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<SessionMediatorReport> const& sm_report,
    std::shared_ptr<EventSink> const& sink,
    std::shared_ptr<Screencast> const& effective_screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images)
{
    return std::make_shared<DebuggingSessionMediator>(
        shell,
        graphics_platform,
        changer,
        buffer_allocator->supported_pixel_formats(),
        sm_report,
        sink,
        resource_cache(),
        effective_screencast,
        connection_context,
        cursor_images);
}
