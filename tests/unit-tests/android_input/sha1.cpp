/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "EventHub.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace android;

namespace
{
char const* const test_text =
R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus luctus, nulla in
iaculis scelerisque, ipsum elit varius neque, nec dapibus magna ligula vitae sem. Aenean
auctor porttitor augue, vitae molestie eros tristique id. Nullam eget nibh nisi, a laoreet
dolor. Pellentesque nulla neque, ultrices nec mollis eget, venenatis ac diam. Phasellus
egestas ultrices facilisis. Cras a nulla at purus facilisis varius. Fusce posuere cursus
libero sed pellentesque. In fringilla nibh ut sapien fermentum gravida. Nam ipsum mauris,
euismod non malesuada eu, posuere ac neque. Cras volutpat, lacus in ultricies auctor, est
neque gravida ante, quis cursus ante nisl vitae massa. Fusce facilisis elementum ante non
vestibulum. In id ullamcorper nunc. Vivamus quis blandit turpis. Nullam nec metus massa.
Cras ultrices molestie eleifend. Pellentesque habitant morbi tristique senectus et netus
et malesuada fames ac turpis egestas.)";
}

TEST(AndroidSha1, test)
{
    EXPECT_STREQ("aa74e4cdc9b384c5e30df0d3ea3cd1121a854ef7",
        c_str(detail::sha1(String8(test_text))));
}
