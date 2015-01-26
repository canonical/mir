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
MIR_FLAGS(Character, uint32_t,
          (Empty, 0),
          (Logical, 1<<0),
          (Idealistic, 1<<1),
          (Englihtened, 1<<2),
          (Curious, 1<<3),
          (Paranoid, 1<<16),
          (Unstable, 1<<17),
          (Reckless, 1<<18),
          (Positive, (1<<4) - 1),
          (Negative, 0xFFFF0000))
using Profile = mir::Flags<Character>;
}

MIR_FLAGS_PRETTY_PRINTER(MirOrientationMode,
                         mir_orientation_mode_portrait,
                         mir_orientation_mode_landscape,
                         mir_orientation_mode_landscape_inverted,
                         mir_orientation_mode_portrait_inverted,
                         mir_orientation_mode_portrait_any,
                         mir_orientation_mode_landscape_any,
                         mir_orientation_mode_any
                        )
using OrientatonModeFlags = mir::Flags<MirOrientationMode>;

enum class AnotherExample : uint8_t
{
    None, Set
};

MIR_FLAGS_PRETTY_PRINTER(AnotherExample,
                         AnotherExample::None,
                         AnotherExample::Set)
using ExampleScopedFlags = mir::Flags<AnotherExample>;

namespace mir
{
namespace ns_inside_mir
{
MIR_FLAGS(Capability, uint8_t, (Pointer, 1<<4),(Touchpad, 1<<3))
using Capabilities = mir::Flags<Capability>;
}
}

namespace nested = mir::ns_inside_mir;
namespace arb = arbitrary_namespace;

TEST(MirFlags,lookup_rules_work_in_mir_nested_namespace)
{
    using namespace testing;
    using namespace mir;
    nested::Capabilities cap = nested::Capability::Pointer | nested::Capability::Touchpad;

    EXPECT_THAT(mir::to_string(cap),Eq("Pointer|Touchpad"));
}

TEST(MirFlags,lookup_rules_work_in_arbitrary_namespace)
{
    using namespace testing;
    using namespace mir;
    arb::Profile empty = arb::Character::Curious & arb::Character::Reckless;

    EXPECT_THAT(mir::to_string(empty),Eq("Empty"));
}

namespace mir
{
TEST(MirFlags,using_namespace_mir_not_needed_inside_mir)
{
    using namespace testing;
    arb::Profile mostly_positive = arb::Character::Curious | arb::Character::Logical;

    EXPECT_THAT(mir::to_string(mostly_positive),Eq("Logical|Curious"));
}
}

TEST(MirFlags,contains_check_works_for_masks)
{
    using namespace testing;
    using namespace mir;
    arb::Profile mostly_positive;
    mostly_positive |= arb::Character::Curious | arb::Character::Logical;

    EXPECT_THAT(contains(mostly_positive,arb::Character::Positive),Eq(false));
    mostly_positive = mostly_positive | arb::Character::Idealistic | arb::Character::Englihtened;

    EXPECT_THAT(contains(mostly_positive,arb::Character::Positive),Eq(true));
}

TEST(MirFlags,toggling_bits)
{
    using namespace testing;
    using namespace mir;
    arb::Profile negative{arb::Character::Negative};

    EXPECT_THAT(contains(negative,arb::Character::Paranoid),Eq(true));
    EXPECT_THAT(contains(negative^arb::Character::Paranoid,arb::Character::Paranoid),Eq(false));
    EXPECT_THAT(contains(~negative,arb::Character::Positive),Eq(true));
}

TEST(MirFlags,pretty_printing_existing_enum)
{
    using namespace testing;
    OrientatonModeFlags flags;

    EXPECT_THAT(to_string(flags),Eq("Empty"));
    EXPECT_THAT(to_string(flags|mir_orientation_mode_portrait_inverted),Eq("mir_orientation_mode_portrait_inverted"));
    EXPECT_THAT(to_string(flags|mir_orientation_mode_landscape_any),
                Eq("mir_orientation_mode_landscape|mir_orientation_mode_landscape_inverted|mir_orientation_mode_landscape_any"));
}


TEST(MirFlags,pretty_printing_existing_scoped_enum)
{
    using namespace testing;
    ExampleScopedFlags flags;

    EXPECT_THAT(to_string(flags),Eq("AnotherExample::None"));
    EXPECT_THAT(to_string(flags|AnotherExample::Set),Eq("AnotherExample::Set"));
}
