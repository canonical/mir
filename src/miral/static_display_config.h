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

// Scale is not supported when compositing. https://github.com/MirServer/mir/issues/552
#define MIR_SCALE_NOT_SUPPORTED

namespace miral
{
class StaticDisplayConfig : public mir::graphics::DisplayConfigurationPolicy
{
public:
    StaticDisplayConfig(std::string const& filename);
    virtual void apply_to(mir::graphics::DisplayConfiguration& conf);
private:

    using Id = std::tuple<mir::graphics::DisplayConfigurationCardId, mir::graphics::DisplayConfigurationOutputId>;
    struct Config
    {
        mir::optional_value<mir::geometry::Point>  position;
        mir::optional_value<mir::geometry::Size>   size;
        mir::optional_value<double> refresh;
        mir::optional_value<float>  scale;
        mir::optional_value<MirOrientation>  orientation;
    };

    static constexpr char const* const output_id = "output_id";
    static constexpr char const* const position = "position";
    static constexpr char const* const mode = "mode";
#ifndef MIR_SCALE_NOT_SUPPORTED
    static constexpr char const* const scale = "scale";
#endif
    static constexpr char const* const orientation = "orientation";


    std::map<Id, Config> config;

    static constexpr char const* const orientation_value[] =
        { "normal", "left", "inverted", "right" };

    static auto as_string(MirOrientation orientation) -> char const*
    {
        return orientation_value[orientation/90];
    }

    static auto as_orientation(std::string const& orientation) -> MirOrientation
    {
        if (orientation == orientation_value[3])
            return mir_orientation_right;

        if (orientation == orientation_value[2])
            return mir_orientation_inverted;

        if (orientation == orientation_value[1])
            return mir_orientation_left;

        return mir_orientation_normal;
    }
};
}

#endif //MIRAL_STATIC_DISPLAY_CONFIG_H_
