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
#include <vector>

namespace mw = mir::wayland;

using namespace testing;

namespace
{
template<typename T>
auto as_span(wl_array const* array) -> std::vector<T>
{
    std::vector<T> result;
    auto const count = array->size / sizeof(T);
    auto const* data = static_cast<T const*>(array->data);
    for (size_t i = 0; i != count; ++i)
        result.push_back(data[i]);
    return result;
}
}

TEST(WlArrayTest, default_constructed_is_empty)
{
    mw::WlArray array;
    EXPECT_THAT(array.data()->size, Eq(0u));
    EXPECT_THAT(array.data()->data, IsNull());
}

TEST(WlArrayTest, push_back_appends_values)
{
    mw::WlArray array;
    array.push_back<uint32_t>(1);
    array.push_back<uint32_t>(2);
    array.push_back<uint32_t>(3);

    EXPECT_THAT(as_span<uint32_t>(array.data()), ElementsAre(1u, 2u, 3u));
}

TEST(WlArrayTest, append_copies_raw_bytes)
{
    mw::WlArray array;
    std::vector<uint16_t> const values{10, 20, 30};
    array.append(values.data(), values.size() * sizeof(uint16_t));

    EXPECT_THAT(as_span<uint16_t>(array.data()), ElementsAre(10, 20, 30));
}

TEST(WlArrayTest, append_with_zero_length_is_noop)
{
    mw::WlArray array;
    array.append(nullptr, 0);
    EXPECT_THAT(array.data()->size, Eq(0u));
}

TEST(WlArrayTest, constructible_from_raw_wl_array_pointer)
{
    wl_array raw;
    wl_array_init(&raw);
    auto* const slot = static_cast<uint32_t*>(wl_array_add(&raw, sizeof(uint32_t)));
    *slot = 7;

    mw::WlArray const array{&raw};
    EXPECT_THAT(as_span<uint32_t>(array.data()), ElementsAre(7u));

    wl_array_release(&raw);
}

TEST(WlArrayTest, copy_constructor_deep_copies)
{
    mw::WlArray original;
    original.push_back<uint32_t>(42);

    mw::WlArray const copy{original};
    EXPECT_THAT(as_span<uint32_t>(copy.data()), ElementsAre(42u));
    EXPECT_THAT(copy.data()->data, Ne(original.data()->data)) << "copy should own separate storage";

    // Mutating the original after the copy shouldn't affect the copy
    original.push_back<uint32_t>(99);
    EXPECT_THAT(as_span<uint32_t>(copy.data()), ElementsAre(42u));
    EXPECT_THAT(as_span<uint32_t>(original.data()), ElementsAre(42u, 99u));
}

TEST(WlArrayTest, copy_assignment_deep_copies)
{
    mw::WlArray a;
    a.push_back<uint32_t>(1);
    mw::WlArray b;
    b.push_back<uint32_t>(2);

    b = a;
    EXPECT_THAT(as_span<uint32_t>(b.data()), ElementsAre(1u));

    a.push_back<uint32_t>(5);
    EXPECT_THAT(as_span<uint32_t>(b.data()), ElementsAre(1u));
}

TEST(WlArrayTest, move_constructor_transfers_storage)
{
    mw::WlArray original;
    original.push_back<uint32_t>(11);
    original.push_back<uint32_t>(22);
    auto* const original_data_ptr = original.data()->data;

    mw::WlArray const moved{std::move(original)};
    EXPECT_THAT(moved.data()->data, Eq(original_data_ptr)) << "move should transfer ownership, not copy";
    EXPECT_THAT(as_span<uint32_t>(moved.data()), ElementsAre(11u, 22u));

    // The moved-from object must be left in a valid, empty, destructible state
    EXPECT_THAT(original.data()->size, Eq(0u));
    EXPECT_THAT(original.data()->data, IsNull());
}

TEST(WlArrayTest, move_assignment_transfers_storage)
{
    mw::WlArray a;
    a.push_back<uint32_t>(1);
    mw::WlArray b;
    b.push_back<uint32_t>(2);
    auto* const a_data_ptr = a.data()->data;

    b = std::move(a);
    EXPECT_THAT(b.data()->data, Eq(a_data_ptr));
    EXPECT_THAT(as_span<uint32_t>(b.data()), ElementsAre(1u));
    EXPECT_THAT(a.data()->size, Eq(0u));
}

TEST(WlArrayTest, self_copy_assignment_is_safe)
{
    mw::WlArray array;
    array.push_back<uint32_t>(1);
    array.push_back<uint32_t>(2);

    // Indirection avoids -Wself-assign-overloaded; the aliasing is the point of the test
    mw::WlArray* const self = &array;
    array = *self;

    EXPECT_THAT(as_span<uint32_t>(array.data()), ElementsAre(1u, 2u));
}

TEST(WlArrayTest, self_move_assignment_is_safe)
{
    mw::WlArray array;
    array.push_back<uint32_t>(1);
    array.push_back<uint32_t>(2);

    // Indirection avoids -Wself-move; the aliasing is the point of the test
    mw::WlArray* const self = &array;
    array = std::move(*self);

    EXPECT_THAT(as_span<uint32_t>(array.data()), ElementsAre(1u, 2u));
}

TEST(WlArrayTest, data_is_usable_as_raw_wl_array_pointer)
{
    mw::WlArray array;
    array.push_back<uint32_t>(123);

    // Simulates handing off to a generated send_*_event(..., wl_array*) function
    wl_array const* raw = array.data();
    ASSERT_THAT(raw->size, Eq(sizeof(uint32_t)));
    EXPECT_THAT(*static_cast<uint32_t const*>(raw->data), Eq(123u));
}
