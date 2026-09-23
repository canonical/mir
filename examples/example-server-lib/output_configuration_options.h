/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SHELL_OUTPUT_CONFIGURATION_OPTIONS_H
#define MIRAL_SHELL_OUTPUT_CONFIGURATION_OPTIONS_H

#include <miral/output_configuration.h>

#include <memory>

namespace mir { class Server; }
namespace miral::live_config { class Store; }

/// Provides the `display` configuration (`layout`, `translucent`, `scale` and
/// `autoscale`) that `miral::display_configuration_options()` supports as
/// command-line options, but implemented here, in the shell, using
/// `miral::live_config` and `miral::OutputConfiguration`.
///
/// This demonstrates that a downstream shell no longer needs the legacy MirAL
/// option handling (nor the mirserver display configuration policies it wraps)
/// to customize the output configuration. As the configuration is live, changes
/// are applied to a running server.
class OutputConfigurationOptions
{
public:
    explicit OutputConfigurationOptions(miral::live_config::Store& config_store);

    void operator()(mir::Server& server) const;

private:
    struct Settings;
    class Strategy;
    class State;

    std::shared_ptr<State> state;
    miral::OutputConfiguration output_configuration;
};

#endif //MIRAL_SHELL_OUTPUT_CONFIGURATION_OPTIONS_H
