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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/client_types.h"
#include "mir/graphics/display_configuration.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;

#define EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(TYPE) \
    EXPECT_EQ(static_cast<mg::DisplayConfigurationOutputType>(mir_display_output_type_##TYPE), \
              mg::DisplayConfigurationOutputType::TYPE)

TEST(ServerClientTypes, display_output_types_match)
{
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(unknown);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(vga);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(dvid);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(dvia);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(composite);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(svideo);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(lvds);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(component);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(ninepindin);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(displayport);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(hdmia);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(hdmib);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(tv);
    EXPECT_DISPLAY_OUTPUT_TYPES_MATCH(edp);
}
