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

#include <mir/wayland/wl_array.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace mw = mir::wayland;

using namespace testing;

namespace
{
template<typename T>
auto as_span(mw::WlArray<T> const& array) -> std::span<T const>
{
    auto const* raw = static_cast<wl_array const*>(array);
    return {static_cast<T const*>(raw->data), raw->size / sizeof(T)};
}
}

TEST(WlArrayTest, default_constructed_is_empty)
{
    mw::WlArray<uint32_t> array;
    auto const* raw = static_cast<wl_array const*>(array);
    EXPECT_THAT(raw->size, Eq(0u));
    EXPECT_THAT(raw->data, IsNull());
}

TEST(WlArrayTest, push_back_appends_values)
{
    mw::WlArray<uint32_t> array;
    array.push_back(1);
    array.push_back(2);
    array.push_back(3);

    EXPECT_THAT(as_span(array), ElementsAre(1u, 2u, 3u));
}

TEST(WlArrayTest, append_bulk_copies_values)
{
    mw::WlArray<uint32_t> array;
    std::vector<uint32_t> const values{10, 20, 30};
    array.append(values.begin(), values.end());

    EXPECT_THAT(as_span(array), ElementsAre(10u, 20u, 30u));
}

TEST(WlArrayTest, append_with_empty_range_is_noop)
{
    mw::WlArray<uint32_t> array;
    uint32_t const* const null_ptr = nullptr;
    array.append(null_ptr, null_ptr);
    EXPECT_THAT(static_cast<wl_array const*>(array)->size, Eq(0u));
}

TEST(WlArrayTest, constructible_from_raw_wl_array_pointer)
{
    wl_array raw;
    wl_array_init(&raw);
    auto* const slot = static_cast<uint32_t*>(wl_array_add(&raw, sizeof(uint32_t)));
    *slot = 7;

    mw::WlArray<uint32_t> const array{&raw};
    EXPECT_THAT(as_span(array), ElementsAre(7u));

    wl_array_release(&raw);
}

TEST(WlArrayTest, constructible_from_pointer_and_count)
{
    uint32_t const values[]{4, 5, 6};
    mw::WlArray<uint32_t> const array{values, 3};

    EXPECT_THAT(as_span(array), ElementsAre(4u, 5u, 6u));
}

TEST(WlArrayTest, constructible_from_initializer_list)
{
    mw::WlArray<uint32_t> const array{1, 2, 3};

    EXPECT_THAT(as_span(array), ElementsAre(1u, 2u, 3u));
}

TEST(WlArrayTest, copy_constructor_deep_copies)
{
    mw::WlArray<uint32_t> original;
    original.push_back(42);

    mw::WlArray<uint32_t> const copy{original};
    EXPECT_THAT(as_span(copy), ElementsAre(42u));
    EXPECT_THAT(static_cast<wl_array const*>(copy)->data, Ne(static_cast<wl_array const*>(original)->data))
        << "copy should own separate storage";

    // Mutating the original after the copy shouldn't affect the copy
    original.push_back(99);
    EXPECT_THAT(as_span(copy), ElementsAre(42u));
    EXPECT_THAT(as_span(original), ElementsAre(42u, 99u));
}

TEST(WlArrayTest, copy_assignment_deep_copies)
{
    mw::WlArray<uint32_t> a;
    a.push_back(1);
    mw::WlArray<uint32_t> b;
    b.push_back(2);

    b = a;
    EXPECT_THAT(as_span(b), ElementsAre(1u));

    a.push_back(5);
    EXPECT_THAT(as_span(b), ElementsAre(1u));
}

TEST(WlArrayTest, self_copy_assignment_is_safe)
{
    mw::WlArray<uint32_t> array;
    array.push_back(1);
    array.push_back(2);

    // Indirection avoids -Wself-assign-overloaded; the aliasing is the point of the test
    mw::WlArray<uint32_t>* const self = &array;
    array = *self;

    EXPECT_THAT(as_span(array), ElementsAre(1u, 2u));
}

TEST(WlArrayTest, move_constructor_transfers_storage)
{
    mw::WlArray<uint32_t> original;
    original.push_back(11);
    original.push_back(22);
    auto* const original_data_ptr = static_cast<wl_array*>(original)->data;

    mw::WlArray<uint32_t> const moved{std::move(original)};
    EXPECT_THAT(static_cast<wl_array const*>(moved)->data, Eq(original_data_ptr))
        << "move should transfer ownership, not copy";
    EXPECT_THAT(as_span(moved), ElementsAre(11u, 22u));

    // The moved-from object must be left in a valid, empty, destructible state
    EXPECT_THAT(static_cast<wl_array const*>(original)->size, Eq(0u));
    EXPECT_THAT(static_cast<wl_array const*>(original)->data, IsNull());
}

TEST(WlArrayTest, move_assignment_transfers_storage)
{
    mw::WlArray<uint32_t> a;
    a.push_back(1);
    mw::WlArray<uint32_t> b;
    b.push_back(2);
    auto* const a_data_ptr = static_cast<wl_array*>(a)->data;

    b = std::move(a);
    EXPECT_THAT(static_cast<wl_array const*>(b)->data, Eq(a_data_ptr));
    EXPECT_THAT(as_span(b), ElementsAre(1u));
    EXPECT_THAT(static_cast<wl_array const*>(a)->size, Eq(0u));
}

TEST(WlArrayTest, self_move_assignment_is_safe)
{
    mw::WlArray<uint32_t> array;
    array.push_back(1);
    array.push_back(2);

    // Indirection avoids -Wself-move; the aliasing is the point of the test
    mw::WlArray<uint32_t>* const self = &array;
    array = std::move(*self);

    EXPECT_THAT(as_span(array), ElementsAre(1u, 2u));
}

TEST(WlArrayTest, usable_as_raw_wl_array_pointer)
{
    mw::WlArray<uint32_t> array;
    array.push_back(123);

    // Simulates handing off to a generated send_*_event(..., wl_array*) function
    wl_array* raw = array;
    ASSERT_THAT(raw->size, Eq(sizeof(uint32_t)));
    EXPECT_THAT(*static_cast<uint32_t const*>(raw->data), Eq(123u));
}

TEST(WlArrayTest, const_reference_yields_const_wl_array_pointer)
{
    mw::WlArray<uint32_t> array;
    array.push_back(9);

    mw::WlArray<uint32_t> const& const_ref = array;
    wl_array const* raw = const_ref;
    EXPECT_THAT(raw->size, Eq(sizeof(uint32_t)));
}

TEST(WlArrayTest, const_reference_requires_explicit_cast_for_mutable_pointer)
{
    // Mirrors real call sites (e.g. text_input_v1.cpp/text_input_v2.cpp) that only have a
    // WlArray<T> const& but must hand a plain wl_array* to a generated send_*_event function.
    // The const-qualified conversion operator only yields wl_array const*, so getting a mutable
    // pointer out of a const WlArray requires an explicit, visible const_cast at the call site.
    mw::WlArray<uint32_t> array;
    array.push_back(9);

    mw::WlArray<uint32_t> const& const_ref = array;
    auto const* const_raw = static_cast<wl_array const*>(const_ref);
    auto* mutable_raw = const_cast<wl_array*>(const_raw); // NOLINT(cppcoreguidelines-pro-type-const-cast)

    EXPECT_THAT(mutable_raw->size, Eq(sizeof(uint32_t)));
}

TEST(WlArrayTest, byte_element_type_holds_opaque_data)
{
    // Mirrors TextInputChange::modifier_map, which is a WlArray<std::byte> built by cloning an
    // external wl_array and never interpreted element-by-element.
    wl_array raw;
    wl_array_init(&raw);
    auto* const slot = static_cast<char*>(wl_array_add(&raw, 3));
    slot[0] = 'a';
    slot[1] = 'b';
    slot[2] = 'c';

    mw::WlArray<std::byte> const array{&raw};
    auto const* copied_raw = static_cast<wl_array const*>(array);
    ASSERT_THAT(copied_raw->size, Eq(3u));
    EXPECT_THAT(std::memcmp(copied_raw->data, "abc", 3), Eq(0));

    wl_array_release(&raw);
}
