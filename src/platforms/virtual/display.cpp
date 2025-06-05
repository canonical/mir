/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "display.h"
#include <mir/graphics/display_configuration.h>
#include "display_configuration.h"
#include <mir/log.h>

#include <utility>

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;

namespace
{
auto build_configuration(std::vector<mgv::VirtualOutputConfig> const& output_sizes)
-> std::unique_ptr<mgv::DisplayConfiguration>
{
    std::vector<mg::DisplayConfigurationOutput> output_configurations;
    for (auto const& output: output_sizes)
    {
        output_configurations.push_back(mgv::DisplayConfiguration::build_output(output));
    }
    return std::make_unique<mgv::DisplayConfiguration>(output_configurations);
}
}

mgv::Display::Display(std::vector<VirtualOutputConfig> const& output_sizes)
    : display_configuration{build_configuration(output_sizes)}
{
}

void mgv::Display::for_each_display_sync_group(std::function<void(DisplaySyncGroup &)> const&)
{
}

std::unique_ptr<mg::DisplayConfiguration> mgv::Display::configuration() const
{
    std::lock_guard lock{mutex};
    return display_configuration->clone();
}

bool mgv::Display::apply_if_configuration_preserves_display_buffers(mir::graphics::DisplayConfiguration const& conf)
{
    configure(conf);
    return true;
}

void mgv::Display::configure(mir::graphics::DisplayConfiguration const& conf)
{
    auto const& new_conf = dynamic_cast<DisplayConfiguration const&>(conf);

    std::lock_guard lock{mutex};
    display_configuration = new_conf.clone();
}

void mgv::Display::register_configuration_change_handler(
    mir::graphics::EventHandlerRegister &,
    mir::graphics::DisplayConfigurationChangeHandler const&)
{
}

void mgv::Display::pause()
{
}

void mgv::Display::resume()
{
}

std::shared_ptr<mg::Cursor> mgv::Display::create_hardware_cursor()
{
    return nullptr;
}
