/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "static_display_config.h"

#include <mir/test/doubles/mock_display_configuration.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace mg  = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
struct StaticDisplayConfig : Test
{
    mtd::MockDisplayConfiguration dc;
    mg::DisplayConfigurationOutput dco;
    mg::UserDisplayConfigurationOutput udco{dco};

    miral::StaticDisplayConfig sdc;
};
}

TEST_F(StaticDisplayConfig, empty_config_file_is_invalid)
{
    std::istringstream empty;

    EXPECT_THROW((sdc.load_config(empty, "")), mir::AbnormalExit);

}