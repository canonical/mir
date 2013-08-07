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

#include <gmock/gmock.h>

namespace mir
{
namespace protobuf
{

//avoid a valgrind complaint by defining printer for this type
class Connection;
void PrintTo(mir::protobuf::Connection const&, ::std::ostream*)
{
}

}
namespace test
{
MATCHER_P2(ClientTypeConfigMatches, config, formats, "")
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
    
            //TODO: display formats currently is just the formats supported by the allocator. should be
            //      the formats the display can handle, once we have interfaces from mg::Display for that
            EXPECT_EQ(0u, arg_display.current_output_format);
            EXPECT_EQ(formats.size(), arg_display.num_output_formats); 
            if (formats.size() != arg_display.num_output_formats) return false;
            for (j = 0u; j < formats.size(); j++)
            {
                EXPECT_EQ(formats[j], static_cast<mir::geometry::PixelFormat>(arg_display.output_formats[j]));
            }
        }
        return true;
    }
    return false;
}

 
MATCHER_P2(ProtobufConfigMatches, config, formats, "")
{
    //ASSERT_ doesn't work in MATCHER_P apparently.
    EXPECT_EQ(config.size(), arg.display_output_size());
    if(config.size() == static_cast<unsigned int>(arg.display_output_size()))
    {
        auto i = 0u;
        for (auto const& info : config) 
        {
            auto& arg_display = arg.display_output(i++);
            EXPECT_EQ(info->connected, arg_display.connected());
            EXPECT_EQ(info->used, arg_display.used());
            EXPECT_EQ(info->id.as_value(), arg_display.output_id());
            EXPECT_EQ(info->card_id.as_value(), arg_display.card_id());
            EXPECT_EQ(info->physical_size_mm.width.as_uint32_t(), arg_display.physical_width_mm()); 
            EXPECT_EQ(info->physical_size_mm.height.as_uint32_t(), arg_display.physical_height_mm()); 
            EXPECT_EQ(static_cast<int>(info->top_left.x.as_uint32_t()), arg_display.position_x()); 
            EXPECT_EQ(static_cast<int>(info->top_left.y.as_uint32_t()), arg_display.position_y());
            EXPECT_EQ(info->current_mode_index, arg_display.current_mode());
            EXPECT_EQ(info->modes.size(), arg_display.mode_size());
            if (info->modes.size() != static_cast<unsigned int>(arg_display.mode_size())) return false;
            auto j = 0u;
            for (auto const& mode : info->modes)
            {
                auto& arg_mode = arg_display.mode(j++);
                EXPECT_EQ(mode.size.width.as_uint32_t(), arg_mode.horizontal_resolution());
                EXPECT_EQ(mode.size.height.as_uint32_t(), arg_mode.vertical_resolution());
                EXPECT_FLOAT_EQ(mode.vrefresh_hz, arg_mode.refresh_rate());
            }

            EXPECT_EQ(0u, arg_display.current_format());
            EXPECT_EQ(formats.size(), arg_display.pixel_format_size()); 
            if (formats.size() != static_cast<unsigned int>(arg_display.pixel_format_size())) return false;
            for (j=0u; j<formats.size(); j++)
            {
                EXPECT_EQ(formats[j], static_cast<mir::geometry::PixelFormat>(arg_display.pixel_format(j)));
            }
        }
        return true;;
    }
    return false;
}
}
}
#endif /* MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_ */
