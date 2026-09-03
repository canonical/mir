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

#include "version_compare.h"

#include <compare>
#include <cstddef>
#include <tuple>

namespace mlc = miral::live_config;

namespace
{
auto is_digit(char c) -> bool
{
    return c >= '0' && c <= '9';
}

// A maximal run of decimal digits, split so it can be compared numerically
// without parsing it into an integer (which could overflow). `digits` holds the
// significant digits with leading zeros removed.
struct NumberToken
{
    std::string_view digits;
    std::size_t leading_zeros;

    // Returns a NumberToken parsed from the start of `text`, along with the
    // remaining unparsed text.
    static auto from_string_view(std::string_view text) -> std::pair<NumberToken, std::string_view>
    {
        std::size_t pos = 0;
        while (pos < text.size() && text[pos] == '0')
            ++pos;

        auto const digits_start = pos;
        while (pos < text.size() && is_digit(text[pos]))
            ++pos;

        return {
            NumberToken{text.substr(digits_start, pos - digits_start), pos},
            text.substr(pos)
        };
    }

    auto operator<=>(NumberToken const& other) const -> std::strong_ordering
    {
        // Order by magnitude first: with leading zeros stripped, the run with
        // more significant digits is the larger number, and equal-length digit
        // runs compare lexically (which matches numeric order). Only then fall
        // back to the leading-zero count, so that distinct strings never compare
        // equal and the overall order stays total.
        return std::tuple{digits.size(), digits, leading_zeros}
           <=> std::tuple{other.digits.size(), other.digits, other.leading_zeros};
    }
};

}

auto mlc::version_compare(std::string_view lhs, std::string_view rhs) -> std::strong_ordering
{
    while (!lhs.empty() && !rhs.empty())
    {
        auto const lhs_starts_with_digit = is_digit(lhs.front());
        auto const rhs_starts_with_digit = is_digit(rhs.front());
        if (lhs_starts_with_digit && rhs_starts_with_digit)
        {
            auto const [lhs_number, lhs_remaining_text] = NumberToken::from_string_view(lhs);
            auto const [rhs_number, rhs_remaining_text] = NumberToken::from_string_view(rhs);

            // Both sides start a number: compare the whole digit runs by value.
            if (auto const cmp = lhs_number <=> rhs_number; cmp != std::strong_ordering::equal)
                return cmp;

            lhs = lhs_remaining_text;
            rhs = rhs_remaining_text;
        }
        // Otherwise compare bytes (unsigned so non-ASCII/UTF-8 bytes order by
        // code unit, independent of locale).
        else if (auto const cmp = static_cast<unsigned char>(lhs.front()) <=> static_cast<unsigned char>(rhs.front());
                 cmp != std::strong_ordering::equal)
        {
            return cmp;
        }
        else
        {
            lhs.remove_prefix(1);
            rhs.remove_prefix(1);
        }
    }

    // Whichever string still has characters left is the longer (and therefore
    // greater) one; if both are exhausted they are equal. At least one of the two
    // remainders is zero, since the loop stops as soon as either is exhausted.
    return lhs.size() <=> rhs.size();
}
