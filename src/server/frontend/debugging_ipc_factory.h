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

#ifndef MIR_FRONTEND_DEBUGGING_IPC_FACTORY_H_
#define MIR_FRONTEND_DEBUGGING_IPC_FACTORY_H_

#include "default_ipc_factory.h"

namespace mir
{
namespace frontend
{
class DebuggingIpcFactory : public DefaultIpcFactory
{
public:
    using DefaultIpcFactory::DefaultIpcFactory;

    std::shared_ptr<detail::DisplayServer> make_mediator(
            std::shared_ptr<Shell> const& shell,
            std::shared_ptr<graphics::Platform> const& graphics_platform,
            std::shared_ptr<DisplayChanger> const& changer,
            std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
            std::shared_ptr<SessionMediatorReport> const& sm_report,
            std::shared_ptr<EventSink> const& sink,
            std::shared_ptr<Screencast> const& effective_screencast,
            ConnectionContext const& connection_context,
            std::shared_ptr<input::CursorImages> const& cursor_images) override;
};

}
}

#endif // MIR_FRONTEND_DEBUGGING_IPC_FACTORY_H_
