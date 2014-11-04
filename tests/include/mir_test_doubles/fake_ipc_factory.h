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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FAKE_IPC_FACTORY_H_
#define MIR_TEST_FAKE_IPC_FACTORY_H_

#include "src/server/frontend/default_ipc_factory.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class FakeIpcFactory : public frontend::DefaultIpcFactory
{
public:
    using frontend::DefaultIpcFactory::DefaultIpcFactory;

    std::shared_ptr<frontend::detail::DisplayServer> make_default_mediator(
        std::shared_ptr<frontend::Shell> const& shell,
        std::shared_ptr<graphics::Platform> const& graphics_platform,
        std::shared_ptr<frontend::DisplayChanger> const& changer,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<frontend::SessionMediatorReport> const& sm_report,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<frontend::Screencast> const& effective_screencast,
        frontend::ConnectionContext const& connection_context,
        std::shared_ptr<graphics::CursorImages> const& cursor_images)
    {
        return frontend::DefaultIpcFactory::make_mediator(
            shell, graphics_platform, changer, buffer_allocator,
            sm_report, sink, effective_screencast, connection_context, cursor_images);
    }

    MOCK_METHOD9(make_mediator, std::shared_ptr<frontend::detail::DisplayServer>(
        std::shared_ptr<frontend::Shell> const& shell,
        std::shared_ptr<graphics::Platform> const& graphics_platform,
        std::shared_ptr<frontend::DisplayChanger> const& changer,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<frontend::SessionMediatorReport> const& sm_report,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<frontend::Screencast> const& effective_screencast,
        frontend::ConnectionContext const& connection_context,
        std::shared_ptr<graphics::CursorImages> const& cursor_images));
};
}
}
}

#endif /* MIR_TEST_FAKE_IPC_FACTORY_H_ */
