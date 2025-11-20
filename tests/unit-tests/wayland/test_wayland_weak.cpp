/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/wayland/lifetime_tracker.h>
#include <mir/wayland/weak.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mw = mir::wayland;

using namespace testing;

class MockResource : public virtual mw::LifetimeTracker
{
public:
    // Expose protected method
    void mark_destroyed() const
    {
        LifetimeTracker::mark_destroyed();
    }
};

template<typename T>
auto operator<<(std::ostream& out, mw::Weak<T> const& weak) -> std::ostream&
{
    return out << "wayland::Weak{" << (weak ? &weak.value() : nullptr) << "}";
}

class WaylandWeakTest : public Test
{
public:
    std::unique_ptr<MockResource> resource{std::make_unique<MockResource>()};
    std::unique_ptr<MockResource> resource_a{std::make_unique<MockResource>()};
    std::unique_ptr<MockResource> resource_b{std::make_unique<MockResource>()};
};

template<typename T>
void check_equality_impl(T const& a, T const& b)
{
    EXPECT_THAT(a == b, Eq(b == a)) << "== not symmetric";
    EXPECT_THAT(a != b, Eq(b != a)) << "!= not symmetric";
    EXPECT_THAT(a != b, Eq(!(a == b))) << "!= not the inverse of ==";
}

MATCHER_P(FullyCheckedEq, a, std::string(negation ? "isn't" : "is") + " equal to " + PrintToString(a))
{
    check_equality_impl(arg, a);
    return arg == a;
}

TEST_F(WaylandWeakTest, returns_resource)
{
    mw::Weak<MockResource> const weak{resource.get()};
    EXPECT_THAT(&weak.value(), Eq(resource.get()));
}

TEST_F(WaylandWeakTest, false_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak{resource.get()};
    ASSERT_THAT(weak, Eq(true));
    resource.reset();
    EXPECT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, can_be_created_with_nullptr)
{
    mw::Weak<MockResource> const weak{nullptr};
    EXPECT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, throws_logic_error_on_access_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak{resource.get()};
    resource.reset();
    EXPECT_THROW(weak.value(), std::logic_error);
}

TEST_F(WaylandWeakTest, throws_logic_error_on_access_of_null)
{
    mw::Weak<MockResource> const weak;
    EXPECT_THROW(weak.value(), std::logic_error);
}

TEST_F(WaylandWeakTest, copies_are_equal)
{
    mw::Weak<MockResource> const weak_a{resource.get()};
    mw::Weak<MockResource> const weak_b{weak_a};
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, equal_after_assignment)
{
    mw::Weak<MockResource> const weak_a{resource.get()};
    mw::Weak<MockResource> weak_b{nullptr};
    weak_b = weak_a;
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, weaks_made_from_the_same_resource_are_equal)
{
    mw::Weak<MockResource> const weak_a{resource.get()};
    mw::Weak<MockResource> const weak_b{resource.get()};
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, default_constructed_weak_equal_to_nullptr_constructed_weak)
{
    mw::Weak<MockResource> const weak_a{};
    mw::Weak<MockResource> const weak_b{nullptr};
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, weaks_made_from_different_resources_not_equal)
{
    mw::Weak<MockResource> const weak_a{resource_a.get()};
    mw::Weak<MockResource> const weak_b{resource_b.get()};
    EXPECT_THAT(weak_a, Not(FullyCheckedEq(weak_b)));
}

TEST_F(WaylandWeakTest, nullptr_weak_not_equal_to_valid_weak)
{
    mw::Weak<MockResource> const weak_a{resource.get()};
    mw::Weak<MockResource> const weak_b{nullptr};
    EXPECT_THAT(weak_a, Not(FullyCheckedEq(weak_b)));
}

TEST_F(WaylandWeakTest, weaks_made_from_the_same_resource_are_equal_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak_a{resource.get()};
    mw::Weak<MockResource> const weak_b{resource.get()};
    resource.reset();
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, weaks_made_from_different_resources_are_equal_after_both_destroyed)
{
    mw::Weak<MockResource> const weak_a{resource_a.get()};
    mw::Weak<MockResource> const weak_b{resource_b.get()};
    resource_a.reset();
    resource_b.reset();
    EXPECT_THAT(weak_a, FullyCheckedEq(weak_b));
}

TEST_F(WaylandWeakTest, weak_not_equal_to_weak_of_new_resource_with_same_address)
{
    auto* const old_resource_ptr = resource.get();
    mw::Weak<MockResource> const weak_a{resource.get()};
    resource->~MockResource();
    new(resource.get()) MockResource();
    auto* const new_resource_ptr = resource.get();
    mw::Weak<MockResource> const weak_b{resource.get()};
    ASSERT_THAT(old_resource_ptr, Eq(new_resource_ptr));
    EXPECT_THAT(weak_a, Not(FullyCheckedEq(weak_b)));
}

TEST_F(WaylandWeakTest, is_raw_resource)
{
    mw::Weak<MockResource> const weak{resource.get()};
    // GTest's Eq() assertion doesn't seem to work with uncopyable types
    EXPECT_THAT(weak.is(*resource), Eq(true));
}

TEST_F(WaylandWeakTest, is_not_different_raw_resource)
{
    mw::Weak<MockResource> const weak_a{resource_a.get()};
    // GTest's Eq() assertion doesn't seem to work with uncopyable types
    EXPECT_THAT(weak_a.is(*resource_b), Eq(false));
}

TEST_F(WaylandWeakTest, weak_cleared_when_resource_marked_as_destroyed)
{
    mw::Weak<MockResource> const weak{resource.get()};
    ASSERT_THAT(weak, Eq(true));
    resource->mark_destroyed();
    EXPECT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, as_nullable_ptr_returns_valid_pointer)
{
    mw::Weak<MockResource> const weak{resource.get()};
    EXPECT_THAT(mw::as_nullable_ptr(weak), Eq(resource.get()));
}

TEST_F(WaylandWeakTest, as_nullable_ptr_returns_nullptr_if_resource_destroyed)
{
    mw::Weak<MockResource> const weak{resource.get()};
    resource.reset();
    EXPECT_THAT(mw::as_nullable_ptr(weak), Eq(nullptr));
}
