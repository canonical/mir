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

namespace miral
{
class StaticDisplayConfig : public mir::graphics::DisplayConfigurationPolicy
{
public:
    StaticDisplayConfig(std::string const& filename);
    virtual void apply_to(mir::graphics::DisplayConfiguration& conf);
private:

    using Id = std::tuple<mir::graphics::DisplayConfigurationCardId, MirOutputType, int>;
    struct Config
    {
        mir::optional_value<mir::geometry::Point>  position;
        mir::optional_value<mir::geometry::Size>   size;
        mir::optional_value<double> refresh;
        mir::optional_value<float>  scale;
        mir::optional_value<MirOrientation>  orientation;
    };

    std::map<Id, Config> config;
};
}

#endif //MIRAL_STATIC_DISPLAY_CONFIG_H_
