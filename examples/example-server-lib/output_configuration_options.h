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

#include <miral/configuration_option.h>
#include <miral/output_configuration.h>

#include <memory>
#include <vector>

namespace mir { class Server; }

/// Provides the `--display-config`, `--translucent`, `--display-scale` and
/// `--display-autoscale` options that `miral::display_configuration_options()`
/// supports, but implemented here, in the shell, using `miral::OutputConfiguration`.
///
/// This demonstrates that a downstream shell no longer needs the legacy MirAL
/// option handling (nor the mirserver display configuration policies it wraps)
/// to customize the output configuration.
class OutputConfigurationOptions
{
public:
    OutputConfigurationOptions();

    void operator()(mir::Server& server) const;

private:
    class Strategy;

    std::shared_ptr<Strategy> strategy;
    miral::OutputConfiguration output_configuration;
    std::vector<miral::ConfigurationOption> options;
};

#endif //MIRAL_SHELL_OUTPUT_CONFIGURATION_OPTIONS_H
