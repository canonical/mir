/*
 * Copyright © 2017 Canonical Ltd.
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

#include "test_window_manager_tools.h"
#include "miral/output.h"

#include <experimental/optional>

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
Rectangle const display_area_a{{20, 30}, {600, 400}};
Rectangle const display_area_b{{620, 0}, {800, 500}};

struct OutputUpdates : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        basic_window_manager.add_session(session);
    }
};
}

TEST_F(OutputUpdates, policy_notified_of_output_creation)
{
    std::experimental::optional<Output> output_a;

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }));

    set_outputs({display_area_a});

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(output_a.value().extents(), Eq(display_area_a));
}

TEST_F(OutputUpdates, policy_notified_of_multiple_outputs)
{
    std::experimental::optional<Output> output_a;
    std::experimental::optional<Output> output_b;

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }))
        .WillOnce(Invoke([&](Output const& output){ output_b = output; }));

    set_outputs({display_area_a, display_area_b});

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(output_a.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_b.value().extents(), Eq(display_area_b));
    EXPECT_FALSE(output_a.value().is_same_output(output_b.value()));
}

TEST_F(OutputUpdates, policy_notified_of_output_update)
{
    std::experimental::optional<Output> output_initial;
    std::experimental::optional<Output> output_original;
    std::experimental::optional<Output> output_updated;

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_initial = output; }));

    set_outputs({display_area_a});

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_output_update(_, _))
        .WillOnce(Invoke([&](Output const& updated, Output const& original){
            output_original = original;
            output_updated = updated;
        }));

    set_outputs({display_area_b});

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_TRUE(output_initial.value().is_same_output(output_original.value()));
    EXPECT_TRUE(output_original.value().is_same_output(output_updated.value()));

    EXPECT_THAT(output_initial.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_original.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_updated.value().extents(), Eq(display_area_b));
}

TEST_F(OutputUpdates, policy_notified_of_output_delete)
{
    std::experimental::optional<Output> output_a;
    std::experimental::optional<Output> output_b;
    std::experimental::optional<Output> output_b_deleted;

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }))
        .WillOnce(Invoke([&](Output const& output){ output_b = output; }));

    set_outputs({display_area_a, display_area_b});

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_output_delete(_))
        .WillOnce(Invoke([&](Output const& output){ output_b_deleted = output; }));

    set_outputs({display_area_a});

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_TRUE(output_b_deleted.value().is_same_output(output_b.value()));
    EXPECT_THAT(output_b_deleted.value().extents(), Eq(output_b.value().extents()));
}
