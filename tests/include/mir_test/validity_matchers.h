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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_VALIDITY_MATCHERS_H_
#define MIR_TEST_VALIDITY_MATCHERS_H_

#include <gmock/gmock.h>

#include "mir_toolkit/mir_client_library.h"

using ::testing::MakePolymorphicMatcher;
using ::testing::MatchResultListener;
using ::testing::NotNull;
using ::testing::PolymorphicMatcher;

class IsValidMatcher {
public:
    // To implement a polymorphic matcher, first define a COPYABLE class
    // that has three members MatchAndExplain(), DescribeTo(), and
    // DescribeNegationTo(), like the following.

    // In this example, we want to use NotNull() with any pointer, so
    // MatchAndExplain() accepts a pointer of any type as its first argument.
    // In general, you can define MatchAndExplain() as an ordinary method or
    // a method template, or even overload it.
    template <typename T>
    bool MatchAndExplain(T* p, MatchResultListener* listener) const;

    // Describes the property of a value matching this matcher.
    void DescribeTo(::std::ostream* os) const { *os << "is valid"; }

    // Describes the property of a value NOT matching this matcher.
    void DescribeNegationTo(::std::ostream* os) const { *os << "is not valid"; }
};

template<>
bool IsValidMatcher::MatchAndExplain(MirConnection* connection, MatchResultListener* listener) const;

template<>
bool IsValidMatcher::MatchAndExplain(MirSurface* surface, MatchResultListener* listener) const;

// To construct a polymorphic matcher, pass an instance of the class
// to MakePolymorphicMatcher().  Note the return type.
inline PolymorphicMatcher<IsValidMatcher> IsValid()
{
    return MakePolymorphicMatcher(IsValidMatcher{});
}

#endif // MIR_TEST_VALIDITY_MATCHERS_H_
