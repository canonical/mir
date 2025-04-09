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

#ifndef MIR_TEST_DOUBLES_FAKE_DISPLAY_H_
#define MIR_TEST_DOUBLES_FAKE_DISPLAY_H_

#include "mir/test/doubles/null_display.h"
#include "mir/fd.h"

#include "mir/geometry/rectangle.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{
class StubDisplayConfig;
class FakeDisplay : public NullDisplay
{
public:
    FakeDisplay();

    explicit FakeDisplay(std::vector<geometry::Rectangle> const& output_rects);

    explicit FakeDisplay(std::vector<mir::graphics::DisplayConfigurationOutput> const& output_configs);

    void for_each_display_sync_group(std::function<void(mir::graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<mir::graphics::DisplayConfiguration> configuration() const override;

    void register_configuration_change_handler(
        mir::graphics::EventHandlerRegister& handlers,
        mir::graphics::DisplayConfigurationChangeHandler const& handler) override;

    bool apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const&) override;
    void configure(mir::graphics::DisplayConfiguration const&) override;

    void emit_configuration_change_event(
        std::shared_ptr<mir::graphics::DisplayConfiguration> const& new_config);

    void wait_for_configuration_change_handler();

private:
    std::shared_ptr<StubDisplayConfig> config;
    std::vector<std::unique_ptr<StubDisplaySyncGroup>> groups;
    Fd const wakeup_trigger;
    std::atomic<bool> handler_called;
    std::mutex mutable configuration_mutex;
};
}
}
}

#endif //MIR_TEST_DOUBLES_FAKE_DISPLAY_H_
