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

#ifndef MIR_TEST_DOUBLES_MOCK_LOGGER_H_
#define MIR_TEST_DOUBLES_MOCK_LOGGER_H_

#include <mir/logging/logger.h>
#include <mir/logging/event.h>
#include <mir/logging/tag.h>

#include <gmock/gmock.h>

#include <string>
#include <ranges>

namespace mir::logging
{
inline void PrintTo(Event const& ev, std::ostream* os)
{
    *os << (ev.tags() | std::views::transform([](Tag const& tag) { return tag::name(tag); }) |
            std::views::join_with(':') | std::ranges::to<std::string>())
        << ": " << ev.message();
}
}

namespace mir
{
namespace test
{
namespace doubles
{

MATCHER_P2(LogMatching, severity_matcher, message_matcher, "")
{
    return testing::ExplainMatchResult(severity_matcher, arg.severity(), result_listener) &&
           testing::ExplainMatchResult(message_matcher, std::string{arg.message()}, result_listener);
}

class IsTagMatcher
{
public:
    using is_gtest_matcher = void;

    IsTagMatcher(mir::logging::Tag const& t) : tag{&t} {}

    bool MatchAndExplain(std::reference_wrapper<mir::logging::Tag const> t, std::ostream*) const
    { return &t.get() == tag; }

    void DescribeTo(std::ostream* os) const { *os << "is the tag " << mir::logging::tag::name(*tag); }

    void DescribeNegationTo(std::ostream* os) const { *os << "is not the tag " << mir::logging::tag::name(*tag); }

private:
    mir::logging::Tag const* const tag;
};

inline auto IsTag(mir::logging::Tag const& tag) -> testing::Matcher<std::reference_wrapper<mir::logging::Tag const>>
{ return IsTagMatcher(tag); }

MATCHER_P3(LogMatching, severity_matcher, message_matcher, tags_matcher, "")
{
    if (!testing::ExplainMatchResult(severity_matcher, arg.severity(), result_listener))
        return false;
    if (!testing::ExplainMatchResult(message_matcher, std::string{arg.message()}, result_listener))
        return false;
    return testing::ExplainMatchResult(tags_matcher, arg.tags(), result_listener);
}

class MockLogger : public logging::Logger
{
public:
    MOCK_METHOD(void, log, (logging::Event const& log_event), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_LOGGER_H_ */
