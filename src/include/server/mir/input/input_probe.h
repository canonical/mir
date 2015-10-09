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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_PROBE_H_
#define MIR_INPUT_PROBE_H_

#include <vector>
#include "mir/module_deleter.h"

namespace mir
{
namespace options
{
class Option;
}
class EmergencyCleanupRegistry;
class SharedLibraryProberReport;

namespace input
{
class InputReport;
class Platform;
class InputDeviceRegistry;

std::vector<mir::UniqueModulePtr<Platform>> probe_input_platforms(
    options::Option const& options, std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<InputDeviceRegistry> const& device_registry, std::shared_ptr<InputReport> const& input_report,
    SharedLibraryProberReport & prober_report);

}
}

#endif
