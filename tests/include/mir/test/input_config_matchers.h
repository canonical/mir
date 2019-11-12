/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */
#ifndef MIR_TEST_INPUT_CONFIG_MATCHERS_H
#define MIR_TEST_INPUT_CONFIG_MATCHERS_H

#include "check_gtest_version.h"
#include "mir/input/mir_input_config.h"
#include <gmock/gmock.h>

namespace testing
{
namespace internal
{

class InputConfigElementsMatcher
    : public MatcherInterface<MirInputConfig const&>,
      public UnorderedElementsAreMatcherImplBase
{
public:
    typedef MirInputDevice Element;

    // Constructs the matcher from a sequence of element values or
    // element matchers.
    template <typename InputIter>
#if GTEST_AT_LEAST(1, 8, 1)
    InputConfigElementsMatcher(UnorderedMatcherRequire::Flags matcher_flags, InputIter first, InputIter last)
        : UnorderedElementsAreMatcherImplBase{matcher_flags}
#else
    InputConfigElementsMatcher(InputIter first, InputIter last)
#endif
    {
        for (; first != last; ++first)
        {
            matchers_.push_back(MatcherCast<const Element&>(*first));
            matcher_describers().push_back(matchers_.back().GetDescriber());
        }
    }
    virtual void DescribeTo(::std::ostream* os) const
    {
        return UnorderedElementsAreMatcherImplBase::DescribeToImpl(os);
    }
    virtual void DescribeNegationTo(::std::ostream* os) const
    {
        return UnorderedElementsAreMatcherImplBase::DescribeNegationToImpl(os);
    }

    virtual bool MatchAndExplain(MirInputConfig const& container, MatchResultListener* listener) const
    {
        ::std::vector<std::string> element_printouts;
        MatchMatrix matrix = AnalyzeElements(container, &element_printouts, listener);

        const size_t actual_count = matrix.LhsSize();
        if (actual_count == 0 && matchers_.empty())
        {
            return true;
        }
        if (actual_count != matchers_.size())
        {
            if (actual_count != 0 && listener->IsInterested())
            {
                *listener << "which has " << Elements(actual_count);
            }
            return false;
        }

#if GTEST_AT_LEAST(1, 8, 1)
        bool matrix_matched = VerifyMatchMatrix(element_printouts, matrix, listener);
#else
        bool matrix_matched = VerifyAllElementsAndMatchersAreMatched(element_printouts, matrix, listener);
#endif

        return matrix_matched && FindPairing(matrix, listener);
    }

private:
    typedef ::std::vector<Matcher<const Element&>> MatcherVec;

    MatchMatrix AnalyzeElements(MirInputConfig const& config,
                                ::std::vector<std::string>* element_printouts,
                                MatchResultListener* listener) const
    {
        element_printouts->clear();
        ::std::vector<char> did_match;
        size_t num_elements = config.size();
        config.for_each(
            [&](MirInputDevice const& element)
            {
                if (listener->IsInterested())
                    element_printouts->push_back(PrintToString(element));
                for (size_t irhs = 0; irhs != matchers_.size(); ++irhs)
                    did_match.push_back(Matches(matchers_[irhs])(element));
            });

        MatchMatrix matrix(num_elements, matchers_.size());
        ::std::vector<char>::const_iterator did_match_iter = did_match.begin();
        for (size_t ilhs = 0; ilhs != num_elements; ++ilhs)
            for (size_t irhs = 0; irhs != matchers_.size(); ++irhs)
                matrix.SetEdge(ilhs, irhs, *did_match_iter++ != 0);
        return matrix;
    }

    MatcherVec matchers_;

    GTEST_DISALLOW_ASSIGN_(InputConfigElementsMatcher);
};

// Multiple specializations because gmock does not decay the parameter type to the
// actual value type.
template <>
class UnorderedElementsAreMatcherImpl<MirInputConfig const&>
    : public InputConfigElementsMatcher
{
public:
    using InputConfigElementsMatcher::InputConfigElementsMatcher;
};

template <>
class UnorderedElementsAreMatcherImpl<MirInputConfig&>
    : public InputConfigElementsMatcher
{
public:
    using InputConfigElementsMatcher::InputConfigElementsMatcher;
};

template <>
class UnorderedElementsAreMatcherImpl<MirInputConfig>
    : public InputConfigElementsMatcher
{
public:
    using InputConfigElementsMatcher::InputConfigElementsMatcher;
};

template <>
class UnorderedElementsAreMatcherImpl<MirInputConfig const>
    : public InputConfigElementsMatcher
{
public:
    using InputConfigElementsMatcher::InputConfigElementsMatcher;
};

}
}

#endif
