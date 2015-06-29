/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/test/display_config_matchers.h"
#include "gtest/gtest.h"

namespace mp = mir::protobuf;
namespace mcl = mir::client;
namespace mt = mir::test;

namespace
{
void fill(mp::DisplayCard* out)
{
    out->set_card_id(7);
    out->set_max_simultaneous_outputs(3);
}

void fill(mp::DisplayOutput* out)
{
    out->add_pixel_format(4);
    out->set_current_format(45);
    auto mode = out->add_mode();
    mode->set_horizontal_resolution(4);
    mode->set_vertical_resolution(558);
    mode->set_refresh_rate(4.33f);
    out->set_current_mode(0);
    out->set_position_x(5);
    out->set_position_y(6);
    out->set_card_id(7);
    out->set_output_id(8);
    out->set_connected(1);
    out->set_used(1);
    out->set_physical_width_mm(11);
    out->set_physical_height_mm(12);
    out->set_orientation(90);
}
}

TEST(TestDisplayConfiguration, configuration_storage)
{
    mp::DisplayConfiguration protobuf_config;
    fill(protobuf_config.add_display_output());
    fill(protobuf_config.add_display_output());
    fill(protobuf_config.add_display_card());
    fill(protobuf_config.add_display_card());
    fill(protobuf_config.add_display_card());

    mcl::DisplayConfiguration internal_config;

    internal_config.update_configuration(protobuf_config);
    MirDisplayConfiguration *info;
    info = internal_config.copy_to_client();

    EXPECT_THAT(*info, mt::DisplayConfigMatches(protobuf_config));
    mcl::delete_config_storage(info);

    int called_count = 0u;
    internal_config.set_display_change_handler([&]() { called_count++; });

    mp::DisplayConfiguration new_result;
    fill(new_result.add_display_output());

    internal_config.update_configuration(new_result);

    info = internal_config.copy_to_client();
    EXPECT_THAT(*info, mt::DisplayConfigMatches(new_result));
    EXPECT_EQ(1u, called_count);

    mcl::delete_config_storage(info);
}
