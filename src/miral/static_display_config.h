/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_STATIC_DISPLAY_CONFIG_H_
#define MIRAL_STATIC_DISPLAY_CONFIG_H_

#include <mir/abnormal_exit.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>

#include <map>
#include <iosfwd>

namespace miral
{
class StaticDisplayConfig : public mir::graphics::DisplayConfigurationPolicy
{
public:
    StaticDisplayConfig();
    StaticDisplayConfig(std::string const& filename);

    virtual void apply_to(mir::graphics::DisplayConfiguration& conf);

    void load_config(std::istream& config_file, std::string const& error_prefix);

    void select_layout(std::string const& layout);

    auto list_layouts() const -> std::vector<std::string>;

private:

    std::string layout = "default";

    using Id = std::tuple<mir::graphics::DisplayConfigurationCardId, MirOutputType, int>;
    struct Config
    {
        bool  disabled = false;
        mir::optional_value<mir::geometry::Point>  position;
        mir::optional_value<mir::geometry::Size>   size;
        mir::optional_value<double> refresh;
        mir::optional_value<float>  scale;
        mir::optional_value<MirOrientation>  orientation;
    };

    using Id2Config = std::map<Id, Config>;
    using Layout2Id2Config = std::map<std::string, Id2Config>;

    Layout2Id2Config config;
};
}

#endif //MIRAL_STATIC_DISPLAY_CONFIG_H_
