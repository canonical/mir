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

#include "mir/test/doubles/fake_display.h"

#include "mir/test/doubles/stub_display_configuration.h"

#include "mir/graphics/event_handler_register.h"

#include <system_error>
#include <boost/throw_exception.hpp>

#include <sys/eventfd.h>

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;

namespace
{
bool compatible(mtd::StubDisplayConfig const& conf1, mtd::StubDisplayConfig const& conf2)
{
    auto compatible =
        conf1.cards == conf2.cards &&
        conf1.outputs.size() == conf2.outputs.size() &&
        std::equal(begin(conf1.outputs), end(conf1.outputs),
                             begin(conf2.outputs),
                             [] (mg::DisplayConfigurationOutput const& output1, mg::DisplayConfigurationOutput const& output2)
                             {
                                 // ignore difference in orientation, scale factor, form factor, subpixel arrangement
                                 auto clone = output2;
                                 clone.orientation = output1.orientation;
                                 clone.subpixel_arrangement = output1.subpixel_arrangement;
                                 clone.scale = output1.scale;
                                 clone.form_factor = output1.form_factor;
                                 return clone == output1;
                             });
    return compatible;
}
}

mtd::FakeDisplay::FakeDisplay()
    : config{std::make_shared<StubDisplayConfig>()},
      wakeup_trigger{::eventfd(0, EFD_CLOEXEC)},
      handler_called{false}
{
    if (wakeup_trigger == Fd::invalid)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to create wakeup FD"));
    }
}

mtd::FakeDisplay::FakeDisplay(std::vector<geometry::Rectangle> const& output_rects) :
    config{std::make_shared<StubDisplayConfig>(output_rects)},
    wakeup_trigger{::eventfd(0, EFD_CLOEXEC)},
    handler_called{false}
{
    if (wakeup_trigger == Fd::invalid)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to create wakeup FD"));
    }
    for (auto const& rect : output_rects)
        groups.emplace_back(new StubDisplaySyncGroup({rect}));
}

mtd::FakeDisplay::FakeDisplay(std::vector<mir::graphics::DisplayConfigurationOutput> const& output_configs) :
    config{std::make_shared<StubDisplayConfig>(output_configs)},
    wakeup_trigger{::eventfd(0, EFD_CLOEXEC)},
    handler_called{false}
{
    if (wakeup_trigger == Fd::invalid)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to create wakeup FD"));
    }
    for (auto const& config : output_configs)
        groups.emplace_back(new StubDisplaySyncGroup({config.extents()}));
}

void mtd::FakeDisplay::for_each_display_sync_group(std::function<void(mir::graphics::DisplaySyncGroup&)> const& f)
{
    std::lock_guard lock{configuration_mutex};
    for (auto& group : groups)
        f(*group);
}

std::unique_ptr<mir::graphics::DisplayConfiguration> mtd::FakeDisplay::configuration() const
{
    std::lock_guard lock{configuration_mutex};
    return std::unique_ptr<mir::graphics::DisplayConfiguration>(new StubDisplayConfig(*config));
}

void mtd::FakeDisplay::register_configuration_change_handler(
    mir::graphics::EventHandlerRegister& handlers,
    mir::graphics::DisplayConfigurationChangeHandler const& handler)
{
    handlers.register_fd_handler(
        {wakeup_trigger},
        this,
        [this, handler](int fd)
            {
                eventfd_t value;
                if (eventfd_read(fd, &value) == -1)
                {
                    BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to read from wakeup FD"));
                }
                if (value > 0)
                {
                    handler();
                    handler_called = true;
                }
            });
}

bool mtd::FakeDisplay::apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const& new_config)
{
    auto new_configuration = std::make_shared<StubDisplayConfig>(new_config);

    std::lock_guard lock{configuration_mutex};
    if (compatible(*config, *new_configuration))
    {
        std::swap(config, new_configuration);
        return true;
    }

    return false;
}

void mtd::FakeDisplay::configure(mir::graphics::DisplayConfiguration const& new_config)
{
    std::lock_guard lock{configuration_mutex};
    decltype(config) new_configuration = std::make_shared<StubDisplayConfig>(new_config);
    decltype(groups) new_groups;

    new_configuration->for_each_output([&](mir::graphics::DisplayConfigurationOutput const& output)
        {
            new_groups.emplace_back(new StubDisplaySyncGroup({output.extents()}));
        });

    swap(config, new_configuration);
    swap(groups, new_groups);
}

void mtd::FakeDisplay::emit_configuration_change_event(
    std::shared_ptr<mir::graphics::DisplayConfiguration> const& new_config)
{
    handler_called = false;
    std::lock_guard lock{configuration_mutex};
    config = std::make_shared<StubDisplayConfig>(*new_config);
    if (eventfd_write(wakeup_trigger, 1) == -1)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to write to wakeup FD"));
    }
}

void mtd::FakeDisplay::wait_for_configuration_change_handler()
{
    while (!handler_called)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
}
