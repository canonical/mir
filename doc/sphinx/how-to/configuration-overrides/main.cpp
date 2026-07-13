/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
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

#include <miral/runner.h>
#include <miral/floating_window_manager.h>
#include <miral/set_window_management_policy.h>

// [doc:config-override:includes]
#include <miral/config_file.h>
#include <miral/live_config_ini_file_with_overrides.h>
#include <miral/live_config_overrides_list.h>
// [doc:config-override:includes-end]

#include <iostream>

using namespace miral;
namespace mlc = miral::live_config;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    // [doc:config-override:store]
    mlc::Key const background_key{"display", "background"};
    mlc::Key const workspaces_key{"display", "workspaces"};

    mlc::IniFileWithOverrides config;

    config.add_string_attribute(
        background_key, "Background colour", "black",
        [](mlc::Key const& key, std::optional<std::string_view> value)
        {
            std::cout << "config: " << key.to_string() << "=" << value.value_or("") << std::endl;
        });

    config.add_strings_attribute(
        workspaces_key, "Workspace names",
        [](mlc::Key const& key, std::optional<std::span<std::string const>> values)
        {
            std::string joined;
            if (values)
                for (auto const& v : *values)
                    joined += (joined.empty() ? "" : ",") + v;
            std::cout << "config: " << key.to_string() << "=" << joined << std::endl;
        });

    config.on_done([] { std::cout << "config: loaded" << std::endl; });
    // [doc:config-override:store-end]

    // [doc:config-override:config-file]
    ConfigFile config_file{
        runner,
        "demo-compositor.conf",
        ConfigFile::Mode::reload_on_change,
        [&config](mlc::OverridesList const& changes) { config.load(changes); },
        ".conf"};
    // [doc:config-override:config-file-end]

    return runner.run_with({set_window_management_policy<FloatingWindowManager>()});
}
