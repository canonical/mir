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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/test/doubles/mock_display.h"

#include "mir/test/doubles/stub_display_configuration.h"

#include "mir/graphics/event_handler_register.h"

#include <unistd.h>

namespace mtd = mir::test::doubles;


mtd::MockDisplay::MockDisplay()
    : config{std::make_shared<StubDisplayConfig>()},
      handler_called{false}
{
}

void mtd::MockDisplay::for_each_display_sync_group(std::function<void(mir::graphics::DisplaySyncGroup&)> const& f)
{
    f(display_sync_group);
}

std::unique_ptr<mir::graphics::DisplayConfiguration> mtd::MockDisplay::configuration() const
{
    return std::unique_ptr<mir::graphics::DisplayConfiguration>(new StubDisplayConfig(*config));
}

void mtd::MockDisplay::register_configuration_change_handler(
    mir::graphics::EventHandlerRegister& handlers,
    mir::graphics::DisplayConfigurationChangeHandler const& handler)
{
    handlers.register_fd_handler(
        {p.read_fd()},
        this,
        [this, handler](int fd)
            {
                char c;
                if (read(fd, &c, 1) == 1)
                {
                    handler();
                    handler_called = true;
                }
            });
}

void mtd::MockDisplay::configure(mir::graphics::DisplayConfiguration const& new_config)
{
    config = std::make_shared<StubDisplayConfig>(new_config);
}

void mtd::MockDisplay::emit_configuration_change_event(
    std::shared_ptr<mir::graphics::DisplayConfiguration> const& new_config)
{
    handler_called = false;
    config = new_config;
    if (write(p.write_fd(), "a", 1)) {}
}

void mtd::MockDisplay::wait_for_configuration_change_handler()
{
    while (!handler_called)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
}
