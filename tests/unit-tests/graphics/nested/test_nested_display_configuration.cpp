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

#include <algorithm>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
struct NestedDisplayConfigurationTest : public ::testing::Test
{
    template<int NoOfOutputs, int NoOfCards>
    MirDisplayConfiguration* build_test_config(
        MirDisplayOutput const (&outputs)[NoOfOutputs],
        MirDisplayCard const (&cards)[NoOfCards])
    {
        auto out_tmp = new MirDisplayOutput[NoOfOutputs];
        std::copy(outputs, outputs+NoOfOutputs, out_tmp);

        auto card_tmp = new MirDisplayCard[NoOfCards];
        std::copy(cards, cards+NoOfCards, card_tmp);

        return new MirDisplayConfiguration{ NoOfOutputs, out_tmp, NoOfCards, card_tmp };
    }

    MirDisplayConfiguration* build_trivial_configuration()
    {
        MirDisplayCard cards[] {{1,1}};
        MirDisplayOutput outputs[] {{
            0,
            0,
            0,
            0,

            0,
            0,
            0,

            0,
            0,
            MirDisplayOutputType(0),

            0,
            0,
            0,
            0,

            0,
            0
        }};

        return build_test_config(outputs, cards);
    }
};

struct MockCardVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationCard const&));
};

struct MockOutputVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationOutput const&));
};

}

TEST_F(NestedDisplayConfigurationTest, empty_configuration_is_read_correctly)
{
    auto empty_configuration = new MirDisplayConfiguration{ 0, nullptr, 0, nullptr };

    mgn::NestedDisplayConfiguration config(empty_configuration);

    config.for_each_card([](mg::DisplayConfigurationCard const&) { FAIL(); });
    config.for_each_output([](mg::DisplayConfigurationOutput const&) { FAIL(); });
}

TEST_F(NestedDisplayConfigurationTest, trivial_configuration_has_one_card)
{
    auto trivial_configuration = build_trivial_configuration();

    mgn::NestedDisplayConfiguration config(trivial_configuration);

    MockCardVisitor cv;

    EXPECT_CALL(cv, f(_)).Times(Exactly(1));
    config.for_each_card([&cv](mg::DisplayConfigurationCard const& card) { cv.f(card); });
}

TEST_F(NestedDisplayConfigurationTest, trivial_configuration_has_one_output)
{
    auto trivial_configuration = build_trivial_configuration();

    mgn::NestedDisplayConfiguration config(trivial_configuration);

    MockOutputVisitor ov;

    EXPECT_CALL(ov, f(_)).Times(Exactly(1));
    config.for_each_output([&ov](mg::DisplayConfigurationOutput const& output) { ov.f(output); });
}
