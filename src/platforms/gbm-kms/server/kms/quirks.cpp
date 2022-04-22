/*
 * Copyright © 2021 Canonical Ltd.
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

#include "quirks.h"

#include "mir/log.h"
#include "mir/options/option.h"
#include "mir/udev/wrapper.h"

#include <vector>
#include <unordered_set>

namespace mgg = mir::graphics::gbm;
namespace mo = mir::options;

namespace
{
char const* quirks_option_name = "driver-quirks";
}

namespace
{
auto value_or(char const* maybe_null_string, char const* value_if_null) -> char const*
{
    if (maybe_null_string)
    {
        return maybe_null_string;
    }
    else
    {
        return value_if_null;
    }
}
}

class mgg::Quirks::Impl
{
public:
    explicit Impl(mo::Option const& options)
    {
        if (!options.is_set(quirks_option_name))
        {
            return;
        }

        for (auto const& quirk : options.get<std::vector<std::string>>(quirks_option_name))
        {
            // Quirk format is skip:(devnode|driver):value

            if (quirk.size() < 5)
            {
                mir::log_warning(
                    "Ignoring unexpected value for %s option: %s (expects value of the form “skip:<type>:<value>”)",
                    quirks_option_name,
                    quirk.c_str());
                continue;
            }
            if (strncmp(quirk.c_str(), "skip:", strlen("skip:")) != 0)
            {
                mir::log_warning(
                    "Ignoring unexpected value for %s option: %s (expects value of the form “skip:<type>:<value>”)",
                    quirks_option_name,
                    quirk.c_str());
                continue;
            }

            auto type_delimeter_pos = quirk.find(':', 5);
            if (type_delimeter_pos == std::string::npos)
            {
                mir::log_warning(
                    "Ignoring unexpected value for %s option: %s (expects value of the form “skip:<type>:<value>”)",
                    quirks_option_name,
                    quirk.c_str());
                continue;
            }
            auto const type = quirk.substr(5, type_delimeter_pos - 5);
            auto const value = quirk.substr(type_delimeter_pos + 1);
            if (type == "devnode")
            {
                devnodes_to_skip.insert(value);
            }
            else if (type == "driver")
            {
                drivers_to_skip.insert(value);
            }
            else
            {
                mir::log_warning(
                    "Ignoring unexpected value for %s option: %s (valid types are “devnode” and “driver”)",
                    quirks_option_name,
                    type.c_str());
                continue;
            }
        }
    }

    auto should_skip(udev::Device const& device) const -> bool
    {
        auto const devnode = value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver =
            [&]()
            {
                if (parent_device)
                {
                    return value_or(parent_device->driver(), "");
                }
                mir::log_warning("udev device has no parent! Unable to determine driver for quirks.");
                return "<UNKNOWN>";
            }();
        mir::log_debug("Quirks: checking device with devnode: %s, driver %s", device.devnode(), driver);
        bool const should_skip_driver = drivers_to_skip.count(driver);
        bool const should_skip_devnode = devnodes_to_skip.count(devnode);
        if (should_skip_driver)
        {
            mir::log_info("Quirks: skipping device %s (matches driver quirk %s)", devnode, driver);
        }
        if (should_skip_devnode)
        {
            mir::log_info("Quirks: skipping device %s (matches devnode quirk %s)", devnode, devnode);
        }
        return should_skip_driver || should_skip_devnode;
    }

private:
    std::unordered_set<std::string> drivers_to_skip;
    std::unordered_set<std::string> devnodes_to_skip;
};

mgg::Quirks::Quirks(const options::Option& options)
    : impl{std::make_unique<Impl>(options)}
{
}

mgg::Quirks::~Quirks() = default;

auto mgg::Quirks::should_skip(udev::Device const& device) const -> bool
{
    return impl->should_skip(device);
}

void mgg::Quirks::add_quirks_option(boost::program_options::options_description& config)
{
    config.add_options()
        (quirks_option_name,
         boost::program_options::value<std::vector<std::string>>(),
         "[platform-specific] Driver quirks to apply (may be specified multiple times; multiple quirks are combined)");
}
