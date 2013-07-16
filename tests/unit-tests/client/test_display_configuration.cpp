/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/client/display_configuration.h"
#include <gtest/gtest.h>

TEST(configuration_struct_empty)
{
    mcl::DisplayConfiguration config;
    EXPECT_EQ(0, config.num_displays);
    EXPECT_EQ(nullptr, config.displays);
}

TEST(configuration_struct_set)
{
    int const num_displays = 4;
    int const num_pfs = 3;
    int const num_modes = 4;

    ///setup message
    mp::DisplayConfiguration msg;
    for(auto i=0u; i < num_displays; i++)
    {
        auto disp = msg.add_display_info();
        for(auto j=0u; j < num_pfs; j++)
        {
            auto pf = disp.add_pixel_format(j);
        }
        disp->set_current_format(1);

        for(auto j=0u; j < num_modes; j++)
        {
            auto mode = disp.add_mode();
            mode->set_horizontal_resolution(6*j); 
            mode->set_vertical_resolution(4*j); 
            mode->set_refresh_rate(j*40.0f); 
        }
        disp->set_current_mode(2);

        disp->set_position_x();
        disp->set_position_y();
        disp->set_card_id();
        disp->set_output_id();
    }
    ///end setup message

    mcl::DisplayConfiguration config;
    config.set_from_message(msg);

    //display_struct
    ASSERT_EQ(msg.display_info().size(), result_info.num_displays);
    ASSERT_NE(nullptr, config.displays);
    for(auto i=0u; i < config.num_display; i++)
    {
        auto msg_disp_info = msg.display_info(i);
        auto result_info = config.display_info[i];

        EXPECT_EQ(msg_disp_info->card_id(), result_info.card_id)
        EXPECT_EQ(msg_disp_info->output_id(), result_info.output_id)
        EXPECT_EQ(msg_disp_info->position_x(), result_info.position_x);
        EXPECT_EQ(msg_disp_info->position_y(), result_info.position_y);

        ASSERT_EQ(msg_disp_info.mode().size(), result_info.num_modes);
        for(auto j=0u; j<result_info.num_modes; j++)
        {
    MirDisplayMode* modes;
            
        }
        EXPECT_EQ(msg_disp_info.current_mode(), result_info.current_mode);
 
        ASSERT_EQ(msg_disp_info.pixel_format().size(), result_info.num_pixel_formats);
        for(auto j=0u; j<result_info.num_modes; j++)
        {
            MirPixelFormat* pixel_formats;
        }
        EXPECT_EQ(msg_disp_info.current_format(), result_info.current_format);
    }
}
