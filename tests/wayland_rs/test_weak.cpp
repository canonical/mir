/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lifetime_tracker.h"
#include "weak.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace mrs = mir::wayland;

using namespace testing;

namespace
{
/// An abstract-style base, mirroring a generated wayland_rs interface class.
class Base : public mrs::LifetimeTracker
{
public:
    virtual ~Base() = default;

    /// A LifetimeTracker subclass can hand out a weak reference to itself without
    /// std::enable_shared_from_this, because the control block is its own destroyed-flag.
    auto weak_to_self() -> mrs::Weak<Base> { return mrs::make_weak(this); }
};

/// A concrete implementation, mirroring a hand-written C++ protocol implementation.
class Concrete : public Base
{
public:
    explicit Concrete(int id) : id{id} {}

    int const id;
};

/// A second, unrelated concrete type used to verify failed downcasts.
class OtherConcrete : public Base
{
};

/// Mirrors the `from<Self>` template that the wayland_rs generator now emits on every generated
/// base class. Kept in sync with cpp_protocol_generation.rs::wayland_interface_to_cpp_class.
class GeneratedBase : public mrs::LifetimeTracker
{
public:
    virtual ~GeneratedBase() = default;

    template<typename Self>
    static auto from(::mir::wayland::Weak<GeneratedBase> const& weak) -> Self*
    {
        return ::mir::wayland::as_nullable_ptr(
            ::mir::wayland::weak_cast<Self>(weak));
    }
};

class GeneratedConcrete : public GeneratedBase
{
public:
    explicit GeneratedConcrete(int id) : id{id} {}
    int const id;
};

class OtherGeneratedConcrete : public GeneratedBase
{
};
}

TEST(WaylandRsWeak, default_constructed_weak_is_falsey)
{
    mrs::Weak<Base> const weak;
    EXPECT_FALSE(weak);
    EXPECT_THAT(mrs::as_nullable_ptr(weak), Eq(nullptr));
}

TEST(WaylandRsWeak, weak_to_live_object_is_truthy_and_resolves)
{
    Concrete object{42};
    auto const weak = mrs::make_weak<Base>(&object);

    EXPECT_TRUE(weak);
    EXPECT_THAT(&weak.value(), Eq(static_cast<Base*>(&object)));
    EXPECT_THAT(mrs::as_nullable_ptr(weak), Eq(static_cast<Base*>(&object)));
}

TEST(WaylandRsWeak, object_can_hand_out_weak_to_itself_without_shared_from_this)
{
    Concrete object{7};
    auto const weak = object.weak_to_self();

    EXPECT_TRUE(weak);
    EXPECT_TRUE(weak.is(object));
    EXPECT_THAT(&weak.value(), Eq(static_cast<Base*>(&object)));
}

TEST(WaylandRsWeak, weak_expires_when_object_is_destroyed)
{
    auto object = std::make_unique<Concrete>(1);
    auto const weak = mrs::make_weak<Base>(object.get());

    ASSERT_TRUE(weak);
    object.reset();

    EXPECT_FALSE(weak);
    EXPECT_THAT(mrs::as_nullable_ptr(weak), Eq(nullptr));
    EXPECT_THROW(weak.value(), std::logic_error);
}

TEST(WaylandRsWeak, shared_ptr_constructor_borrows_without_owning)
{
    auto object = std::make_shared<Concrete>(5);
    mrs::Weak<Base> const weak{std::static_pointer_cast<Base>(object)};

    ASSERT_TRUE(weak);
    EXPECT_THAT(object.use_count(), Eq(1)) << "Weak must not keep the object alive";

    object.reset();
    EXPECT_FALSE(weak);
}

TEST(WaylandRsWeak, equality_and_is_use_object_identity)
{
    Concrete a{1};
    Concrete b{2};

    auto const weak_a = mrs::make_weak<Base>(&a);
    auto const weak_a_again = mrs::make_weak<Base>(&a);
    auto const weak_b = mrs::make_weak<Base>(&b);
    mrs::Weak<Base> const null_weak;

    EXPECT_TRUE(weak_a == weak_a_again);
    EXPECT_FALSE(weak_a != weak_a_again);
    EXPECT_TRUE(weak_a != weak_b);
    EXPECT_TRUE(null_weak == mrs::Weak<Base>{});

    EXPECT_TRUE(weak_a.is(a));
    EXPECT_FALSE(weak_a.is(b));
    EXPECT_FALSE(null_weak.is(a));
}

TEST(WaylandRsWeak, as_downcasts_base_to_concrete)
{
    Concrete object{99};
    auto const base_weak = mrs::make_weak<Base>(&object);

    auto const concrete_weak = base_weak.as<Concrete>();
    ASSERT_TRUE(concrete_weak);
    EXPECT_THAT(concrete_weak.value().id, Eq(99));
    EXPECT_THAT(&concrete_weak.value(), Eq(&object));
}

TEST(WaylandRsWeak, weak_cast_downcasts_base_to_concrete)
{
    Concrete object{12};
    auto const base_weak = mrs::make_weak<Base>(&object);

    auto const concrete_weak = mrs::weak_cast<Concrete>(base_weak);
    ASSERT_TRUE(concrete_weak);
    EXPECT_THAT(concrete_weak.value().id, Eq(12));
}

TEST(WaylandRsWeak, downcast_to_wrong_type_yields_empty_weak)
{
    OtherConcrete object;
    auto const base_weak = mrs::make_weak<Base>(&object);

    ASSERT_TRUE(base_weak);
    EXPECT_FALSE(base_weak.as<Concrete>());
    EXPECT_FALSE(mrs::weak_cast<Concrete>(base_weak));
}

TEST(WaylandRsWeak, downcast_of_expired_weak_yields_empty_weak)
{
    auto object = std::make_unique<Concrete>(3);
    auto const base_weak = mrs::make_weak<Base>(object.get());
    object.reset();

    EXPECT_FALSE(base_weak.as<Concrete>());
    EXPECT_FALSE(mrs::weak_cast<Concrete>(base_weak));
}

// The following mirror the generator-emitted `from<Self>` template (see GeneratedBase above).

TEST(WaylandRsWeak, generated_from_recovers_concrete_pointer)
{
    GeneratedConcrete object{77};
    auto const base_weak = mrs::make_weak<GeneratedBase>(&object);

    // Both the direct and inherited (via the concrete) spellings must work.
    EXPECT_THAT(GeneratedBase::from<GeneratedConcrete>(base_weak), Eq(&object));
    EXPECT_THAT(GeneratedConcrete::from<GeneratedConcrete>(base_weak), Eq(&object));
}

TEST(WaylandRsWeak, generated_from_returns_nullptr_for_wrong_type)
{
    OtherGeneratedConcrete object;
    auto const base_weak = mrs::make_weak<GeneratedBase>(&object);

    EXPECT_THAT(GeneratedBase::from<GeneratedConcrete>(base_weak), Eq(nullptr));
}

TEST(WaylandRsWeak, generated_from_returns_nullptr_for_expired_weak)
{
    auto object = std::make_unique<GeneratedConcrete>(9);
    auto const base_weak = mrs::make_weak<GeneratedBase>(object.get());
    object.reset();

    EXPECT_THAT(GeneratedBase::from<GeneratedConcrete>(base_weak), Eq(nullptr));
}
