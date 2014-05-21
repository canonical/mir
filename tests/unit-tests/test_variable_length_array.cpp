/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/variable_length_array.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(VariableLengthArray, has_correct_size_if_using_builtin)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_builtin{builtin_size - 1};
    mir::VariableLengthArray<builtin_size> vla{vla_size_builtin};

    EXPECT_THAT(vla.size(), Eq(vla_size_builtin));
    memset(vla.data(), 0x55, vla.size());
}

TEST(VariableLengthArray, has_correct_size_if_using_heap)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_heap{builtin_size + 1};
    mir::VariableLengthArray<builtin_size> vla{vla_size_heap};

    EXPECT_THAT(vla.size(), Eq(vla_size_heap));
    memset(vla.data(), 0x55, vla.size());
}

TEST(VariableLengthArray, resizes_from_builtin_to_heap)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_builtin{builtin_size - 1};
    size_t const vla_size_heap{builtin_size + 1};
    mir::VariableLengthArray<builtin_size> vla{vla_size_builtin};

    vla.resize(vla_size_heap);

    EXPECT_THAT(vla.size(), Eq(vla_size_heap));
    memset(vla.data(), 0x55, vla.size());
}

TEST(VariableLengthArray, resizes_from_builtin_to_builtin)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_builtin1{builtin_size - 1};
    size_t const vla_size_builtin2{builtin_size};
    mir::VariableLengthArray<builtin_size> vla{vla_size_builtin1};

    vla.resize(vla_size_builtin2);

    EXPECT_THAT(vla.size(), Eq(vla_size_builtin2));
    memset(vla.data(), 0x55, vla.size());
}

TEST(VariableLengthArray, resizes_from_heap_to_builtin)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_builtin{builtin_size - 1};
    size_t const vla_size_heap{builtin_size + 1};
    mir::VariableLengthArray<builtin_size> vla{vla_size_heap};

    vla.resize(vla_size_builtin);

    EXPECT_THAT(vla.size(), Eq(vla_size_builtin));
    memset(vla.data(), 0x55, vla.size());
}

TEST(VariableLengthArray, resizes_from_heap_to_heap)
{
    using namespace testing;

    size_t const builtin_size{100};
    size_t const vla_size_heap1{builtin_size + 1};
    size_t const vla_size_heap2{builtin_size + 2};
    mir::VariableLengthArray<builtin_size> vla{vla_size_heap1};

    vla.resize(vla_size_heap2);

    EXPECT_THAT(vla.size(), Eq(vla_size_heap2));
    memset(vla.data(), 0x55, vla.size());
}
