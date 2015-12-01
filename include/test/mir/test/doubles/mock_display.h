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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_H_

#include "mir/test/doubles/null_display.h"
#include "mir/test/pipe.h"

#include "mir/geometry/rectangle.h"

#include <atomic>
#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{
class MockDisplay : public NullDisplay
{
public:
    MockDisplay();

    explicit MockDisplay(std::vector<geometry::Rectangle> const& output_rects);

    void for_each_display_sync_group(std::function<void(mir::graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<mir::graphics::DisplayConfiguration> configuration() const override;

    void register_configuration_change_handler(
        mir::graphics::EventHandlerRegister& handlers,
        mir::graphics::DisplayConfigurationChangeHandler const& handler) override;

    void configure(mir::graphics::DisplayConfiguration const&) override;

    void emit_configuration_change_event(
        std::shared_ptr<mir::graphics::DisplayConfiguration> const& new_config);

    void wait_for_configuration_change_handler();

private:
    std::shared_ptr<mir::graphics::DisplayConfiguration> config;
    std::vector<std::unique_ptr<StubDisplaySyncGroup>> groups;
    Pipe p;
    std::atomic<bool> handler_called;
};
}
}
}

#endif //MIR_TEST_DOUBLES_MOCK_DISPLAY_H_
