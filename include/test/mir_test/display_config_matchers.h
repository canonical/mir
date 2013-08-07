/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_
#define MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_

#include "mir_protobuf.pb.h"
#include "mir_toolkit/client_types.h"
#include "mir/geometry/pixel_format.h"
#include <gmock/gmock.h>

//avoid a valgrind complaint by defining printer for this type
static void PrintTo(MirDisplayConfiguration const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(MirDisplayConfiguration const&, ::std::ostream*)
{
}

namespace mir
{
namespace protobuf
{

class Connection;
static void PrintTo(mir::protobuf::DisplayConfiguration const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(mir::protobuf::DisplayConfiguration const&, ::std::ostream*) {}

static void PrintTo(mir::protobuf::Connection const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(mir::protobuf::Connection const&, ::std::ostream*)
{
}

}
namespace test
{
MATCHER_P(ClientTypeConfigMatches, config, "")
{
    //ASSERT_ doesn't work in MATCHER_P apparently.
    EXPECT_EQ(config.size(), arg->num_displays);
    if (arg->num_displays == config.size())
    {
        auto i = 0u;
        for (auto const& info : config) 
        {
            auto& arg_display = arg->displays[i++];
            EXPECT_EQ(info.connected, arg_display.connected);
            EXPECT_EQ(info.used, arg_display.used);
            EXPECT_EQ(info.id.as_value(), static_cast<int>(arg_display.output_id));
            EXPECT_EQ(info.card_id.as_value(), static_cast<int>(arg_display.card_id));
            EXPECT_EQ(info.physical_size_mm.width.as_uint32_t(), arg_display.physical_width_mm); 
            EXPECT_EQ(info.physical_size_mm.height.as_uint32_t(), arg_display.physical_height_mm); 
            EXPECT_EQ(static_cast<int>(info.top_left.x.as_uint32_t()), arg_display.position_x); 
            EXPECT_EQ(static_cast<int>(info.top_left.y.as_uint32_t()), arg_display.position_y);
            //modes
            EXPECT_EQ(info.current_mode_index, arg_display.current_mode);
            EXPECT_EQ(info.modes.size(), arg_display.num_modes);
            if (info.modes.size() != arg_display.num_modes) return false;
            auto j = 0u;
            for (auto const& mode : info.modes)
            {
                auto const& arg_mode = arg_display.modes[j++];
                EXPECT_EQ(mode.size.width.as_uint32_t(), arg_mode.horizontal_resolution);
                EXPECT_EQ(mode.size.height.as_uint32_t(), arg_mode.vertical_resolution);
                EXPECT_FLOAT_EQ(mode.vrefresh_hz, arg_mode.refresh_rate);
            }
    
            EXPECT_EQ(info.current_format_index, arg_display.current_output_format);
            EXPECT_EQ(info.pixel_formats.size(), arg_display.num_output_formats); 
            if (info.pixel_formats.size() != arg_display.num_output_formats) return false;
            for (j = 0u; j < info.pixel_formats.size(); j++)
            {
                EXPECT_EQ(info.pixel_formats[j], static_cast<mir::geometry::PixelFormat>(arg_display.output_formats[j]));
            }
        }
        return true;
    }
    return false;
}

MATCHER_P(ProtobufConfigMatches, config, "")
{
    //ASSERT_ doesn't work in MATCHER_P apparently.
    EXPECT_EQ(config.size(), arg.display_output_size());
    if(config.size() == static_cast<unsigned int>(arg.display_output_size()))
    {
        auto i = 0u;
        for (auto const& info : config) 
        {
            auto& arg_display = arg.display_output(i++);
            EXPECT_EQ(info.connected, arg_display.connected());
            EXPECT_EQ(info.used, arg_display.used());
            EXPECT_EQ(info.id.as_value(), arg_display.output_id());
            EXPECT_EQ(info.card_id.as_value(), arg_display.card_id());
            EXPECT_EQ(info.physical_size_mm.width.as_uint32_t(), arg_display.physical_width_mm()); 
            EXPECT_EQ(info.physical_size_mm.height.as_uint32_t(), arg_display.physical_height_mm()); 
            EXPECT_EQ(static_cast<int>(info.top_left.x.as_uint32_t()), arg_display.position_x()); 
            EXPECT_EQ(static_cast<int>(info.top_left.y.as_uint32_t()), arg_display.position_y());
            EXPECT_EQ(info.current_mode_index, arg_display.current_mode());
            EXPECT_EQ(info.modes.size(), arg_display.mode_size());
            if (info.modes.size() != static_cast<unsigned int>(arg_display.mode_size())) return false;
            auto j = 0u;
            for (auto const& mode : info.modes)
            {
                auto& arg_mode = arg_display.mode(j++);
                EXPECT_EQ(mode.size.width.as_uint32_t(), arg_mode.horizontal_resolution());
                EXPECT_EQ(mode.size.height.as_uint32_t(), arg_mode.vertical_resolution());
                EXPECT_FLOAT_EQ(mode.vrefresh_hz, arg_mode.refresh_rate());
            }

            EXPECT_EQ(info.current_format_index, arg_display.current_format());
            EXPECT_EQ(info.pixel_formats.size(), arg_display.pixel_format_size()); 
            if (info.pixel_formats.size() != static_cast<unsigned int>(arg_display.pixel_format_size())) return false;
            for ( j = 0u; j < info.pixel_formats.size(); j++)
            {
                EXPECT_EQ(info.pixel_formats[j], static_cast<mir::geometry::PixelFormat>(arg_display.pixel_format(j)));
            }
        }
        return true;;
    }
    return false;
}

MATCHER_P(ClientTypeMatchesProtobuf, config, "")
{
    //ASSERT_ doesn't work in MATCHER_P apparently.
    EXPECT_EQ(config.display_output_size(), arg.num_displays);
    if (config.display_output_size() == static_cast<int>(arg.num_displays))
    {
        for (auto i = 0u; i < arg.num_displays; i++) 
        {
            auto& conf_display = config.display_output(i);
            auto& arg_display = arg.displays[i];
            EXPECT_EQ(conf_display.connected(), arg_display.connected);
            EXPECT_EQ(conf_display.used(), arg_display.used);
            EXPECT_EQ(conf_display.output_id(), arg_display.output_id);
            EXPECT_EQ(conf_display.card_id(), arg_display.card_id);
            EXPECT_EQ(conf_display.physical_width_mm(), arg_display.physical_width_mm); 
            EXPECT_EQ(conf_display.physical_height_mm(), arg_display.physical_height_mm); 
            EXPECT_EQ(conf_display.position_x(), arg_display.position_x); 
            EXPECT_EQ(conf_display.position_y(), arg_display.position_y);

            EXPECT_EQ(conf_display.current_mode(), arg_display.current_mode);
            EXPECT_EQ(conf_display.mode_size(), arg_display.num_modes);
            if (conf_display.mode_size() != static_cast<int>(arg_display.num_modes)) return false;
            for (auto j = 0u; j <  arg_display.num_modes; j++)
            {
                auto const& conf_mode = conf_display.mode(j);
                auto const& arg_mode = arg_display.modes[j];
                EXPECT_EQ(conf_mode.horizontal_resolution(), arg_mode.horizontal_resolution);
                EXPECT_EQ(conf_mode.vertical_resolution(), arg_mode.vertical_resolution);
                EXPECT_FLOAT_EQ(conf_mode.refresh_rate(), arg_mode.refresh_rate);
            }

            EXPECT_EQ(conf_display.current_format(), arg_display.current_output_format);
            EXPECT_EQ(conf_display.pixel_format_size(), arg_display.num_output_formats); 
            if (conf_display.pixel_format_size() != static_cast<int>(arg_display.num_output_formats)) return false;
            for (auto j = 0u; j <  arg_display.num_output_formats; j++)
            {
                EXPECT_EQ(conf_display.pixel_format(j), arg_display.output_formats[j]);
            }
        }
        return true;
    }
    return false;
}


}
}
#endif /* MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_ */
