/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Chase Douglas <chase.douglas@canonical.com>
 */

#include "mir/input/filter.h"

#include <gtest/gtest.h>

namespace mi = mir::input;

TEST(filter, always_continue_filter)
{
    mi::AlwaysContinueFilter<mi::Filter> acf;

    EXPECT_EQ(mi::Filter::Result::continue_processing, acf.accept(nullptr));
}

TEST(filter, always_stop_filter)
{
    mi::AlwaysStopFilter<mi::Filter> acf;

    EXPECT_EQ(mi::Filter::Result::stop_processing, acf.accept(nullptr));
}
