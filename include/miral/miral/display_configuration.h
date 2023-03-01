/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_DISPLAY_CONFIGURATION_H
#define MIRAL_DISPLAY_CONFIGURATION_H

#include <memory>
#include <string>
#include <vector>

namespace mir { class Server; }

namespace miral
{
class MirRunner;
class ConfigurationOption;

/// Enable display configuration.
/// The config file (miral::MirRunner::display_config_file()) is located via
/// the XDG Base Directory Specification. Vis:
///($XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS)
/// \remark Since MirAL 2.4
/// \note From MirAL 3.8 will monitor the configuration file or, if none found,
/// for the creation of a file in $XDG_CONFIG_HOME or $HOME/.config. Changes
/// to this file will be reloaded. In addition, the selected layout may be
/// overridden using a corresponding file: display_config_file() + "-layout"
/// which will also be monitored and changes reloaded
class DisplayConfiguration
{
public:
    explicit DisplayConfiguration(MirRunner const& mir_runner);

    /// Provide the default 'display-layout' configuration option
    auto layout_option() -> ConfigurationOption;

    /// Select a layout from the configuration
    void select_layout(std::string const& layout);

    /// List all layouts found in the config file
    auto list_layouts() -> std::vector<std::string>;

    /// Enable a custom output attribute in the .display YAML
    /// \remark Since MirAL 3.8
    void add_output_attribute(std::string const& key);

    void operator()(mir::Server& server) const;

    ~DisplayConfiguration();
    DisplayConfiguration(DisplayConfiguration const&);
    auto operator=(DisplayConfiguration const&) -> DisplayConfiguration&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_DISPLAY_CONFIGURATION_H
