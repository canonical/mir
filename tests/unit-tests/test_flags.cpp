/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/flags.h"
#include "mir_toolkit/common.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace arbitrary_namespace
{
enum class Character : uint32_t
{
    Empty = 0,
    Logical = 1<<0,
    Idealistic = 1<<1,
    Englihtened = 1<<2,
    Curious = 1<<3,
    Paranoid = 1<<16,
    Unstable = 1<<17,
    Reckless = 1<<18,
    Positive = (1<<4) - 1,
    Negative = 0xFFFF0000
};

Character mir_enable_enum_bit_operators(Character);
using Profile = mir::Flags<Character>;
}

namespace mir
{
namespace ns_inside_mir
{
enum class Capability : uint8_t { Pointer = 1<<4, Touchpad = 1<<3};
Capability mir_enable_enum_bit_operators(Capability);
using Capabilities = mir::Flags<Capability>;
}
}

namespace nested = mir::ns_inside_mir;
namespace arb = arbitrary_namespace;

TEST(MirFlags,lookup_rules_work_in_mir_nested_namespace)
{
    using namespace testing;
    nested::Capabilities cap = nested::Capability::Pointer | nested::Capability::Touchpad;

    EXPECT_THAT(cap.value(), (1<<3) | (1<<4));
}

TEST(MirFlags,lookup_rules_work_in_arbitrary_namespace)
{
    using namespace testing;
    arb::Profile empty = arb::Character::Curious & arb::Character::Reckless;

    EXPECT_THAT(empty.value(),Eq(0));
}

TEST(MirFlags,contains_check_works_for_masks)
{
    using namespace testing;
    arb::Profile mostly_positive;
    mostly_positive |= arb::Character::Curious | arb::Character::Logical;

    EXPECT_THAT(contains(mostly_positive,arb::Character::Positive),Eq(false));
    mostly_positive = mostly_positive | arb::Character::Idealistic | arb::Character::Englihtened;

    EXPECT_THAT(contains(mostly_positive,arb::Character::Positive),Eq(true));
}

TEST(MirFlags, toggling_bits)
{
    using namespace testing;
    arb::Profile negative{arb::Character::Negative};

    EXPECT_THAT(contains(negative,arb::Character::Paranoid),Eq(true));
    EXPECT_THAT(contains(negative^arb::Character::Paranoid,arb::Character::Paranoid),Eq(false));
}
