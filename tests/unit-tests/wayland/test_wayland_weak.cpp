/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/wayland/wayland_base.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <experimental/optional>

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
    std::experimental::optional<MockResource> resource{{}};
    std::experimental::optional<MockResource> resource_a{{}};
    std::experimental::optional<MockResource> resource_b{{}};
};

TEST_F(WaylandWeakTest, returns_resource)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    ASSERT_THAT(&weak.value(), Eq(&resource.value()));
}

TEST_F(WaylandWeakTest, false_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    ASSERT_THAT(weak, Eq(true));
    resource = std::experimental::nullopt;
    ASSERT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, can_be_created_with_nullptr)
{
    mw::Weak<MockResource> const weak{nullptr};
    ASSERT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, throws_logic_error_on_access_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    resource = std::experimental::nullopt;
    EXPECT_THROW(weak.value(), std::logic_error);
}

TEST_F(WaylandWeakTest, throws_logic_error_on_access_of_null)
{
    mw::Weak<MockResource> const weak;
    EXPECT_THROW(weak.value(), std::logic_error);
}

TEST_F(WaylandWeakTest, copies_are_equal)
{
    mw::Weak<MockResource> const weak_a{&resource.value()};
    mw::Weak<MockResource> const weak_b{weak_a};
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, equal_after_assignment)
{
    mw::Weak<MockResource> const weak_a{&resource.value()};
    mw::Weak<MockResource> weak_b{nullptr};
    weak_b = weak_a;
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, weaks_made_from_the_same_resource_are_equal)
{
    mw::Weak<MockResource> const weak_a{&resource.value()};
    mw::Weak<MockResource> const weak_b{&resource.value()};
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, default_constructed_weak_equal_to_nullptr_constructed_weak)
{
    mw::Weak<MockResource> const weak_a{};
    mw::Weak<MockResource> const weak_b{nullptr};
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, weaks_made_from_different_resources_not_equal)
{
    mw::Weak<MockResource> const weak_a{&resource_a.value()};
    mw::Weak<MockResource> const weak_b{&resource_b.value()};
    EXPECT_THAT(weak_a, Ne(weak_b));
    EXPECT_THAT(weak_b, Ne(weak_a));
}

TEST_F(WaylandWeakTest, nullptr_weak_not_equal_to_valid_weak)
{
    mw::Weak<MockResource> const weak_a{&resource.value()};
    mw::Weak<MockResource> const weak_b{nullptr};
    EXPECT_THAT(weak_a, Ne(weak_b));
    EXPECT_THAT(weak_b, Ne(weak_a));
}

TEST_F(WaylandWeakTest, weaks_made_from_the_same_resource_are_equal_after_resource_destroyed)
{
    mw::Weak<MockResource> const weak_a{&resource.value()};
    mw::Weak<MockResource> const weak_b{&resource.value()};
    resource = std::experimental::nullopt;
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, weaks_made_from_different_resources_are_equal_after_both_destroyed)
{
    mw::Weak<MockResource> const weak_a{&resource_a.value()};
    mw::Weak<MockResource> const weak_b{&resource_b.value()};
    resource_a = std::experimental::nullopt;
    resource_b = std::experimental::nullopt;
    EXPECT_THAT(weak_a, Eq(weak_b));
    EXPECT_THAT(weak_b, Eq(weak_a));
}

TEST_F(WaylandWeakTest, weak_not_equal_to_weak_of_new_resource_with_same_address)
{
    auto* const old_resource_ptr = &resource.value();
    mw::Weak<MockResource> const weak_a{&resource.value()};
    resource = std::experimental::nullopt;
    resource.emplace();
    auto* const new_resource_ptr = &resource.value();
    mw::Weak<MockResource> const weak_b{&resource.value()};
    ASSERT_THAT(old_resource_ptr, Eq(new_resource_ptr));
    EXPECT_THAT(weak_a, Ne(weak_b));
    EXPECT_THAT(weak_b, Ne(weak_a));
}

TEST_F(WaylandWeakTest, is_equal_to_raw_resource)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    // GTest's Eq() assertion doesn't seem to work with uncopyable types
    EXPECT_THAT(weak == resource.value(), Eq(true));
    EXPECT_THAT(weak != resource.value(), Eq(false));
}

TEST_F(WaylandWeakTest, not_equal_to_different_raw_resource)
{
    mw::Weak<MockResource> const weak_a{&resource_a.value()};
    // GTest's Eq() assertion doesn't seem to work with uncopyable types
    EXPECT_THAT(weak_a == resource_b.value(), Eq(false));
    EXPECT_THAT(weak_a != resource_b.value(), Eq(true));
}

TEST_F(WaylandWeakTest, weak_cleared_when_resource_marked_as_destroyed)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    ASSERT_THAT(weak, Eq(true));
    resource.value().mark_destroyed();
    EXPECT_THAT(weak, Eq(false));
}

TEST_F(WaylandWeakTest, as_nullable_ptr_returns_valid_pointer)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    ASSERT_THAT(mw::as_nullable_ptr(weak), Eq(&resource.value()));
}

TEST_F(WaylandWeakTest, as_nullable_ptr_returns_nullptr_if_resource_destroyed)
{
    mw::Weak<MockResource> const weak{&resource.value()};
    resource = std::experimental::nullopt;
    ASSERT_THAT(mw::as_nullable_ptr(weak), Eq(nullptr));
}
