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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/graphics/nested/nested_display_configuration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

namespace
{
struct NestedDisplayConfigurationTest : public ::testing::Test
{
};

}

TEST_F(NestedDisplayConfigurationTest, empty_configuration_is_read_correctly)
{
    MirDisplayConfiguration mir_display_configuration{ 0, nullptr, 0, nullptr };

    mgn::NestedDisplayConfiguration config(mir_display_configuration);

    config.for_each_card([](mg::DisplayConfigurationCard const&) { FAIL(); });
    config.for_each_output([](mg::DisplayConfigurationOutput const&) { FAIL(); });
}
