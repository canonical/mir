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

#ifndef MIRAL_VERSION_COMPARE_H
#define MIRAL_VERSION_COMPARE_H

#include <compare>
#include <map>
#include <string>
#include <string_view>

namespace miral::live_config
{
/// Version-aware ("natural") comparison of two strings.
///
/// Maximal runs of decimal digits ([0-9]) are compared by numeric value rather
/// than by byte value, so e.g. "2" orders before "10". Everything else is
/// compared byte-wise (locale-independent). This matches the convention used by
/// drop-in config directories (systemd, udev, ...) where files carry a numeric
/// prefix such as "10-foo.conf" that should sort by value, not by raw bytes.
///
/// Ties on numeric value are broken by the number of leading zeros (fewer first)
/// so the result is a total order: distinct strings never compare equal.
///
/// \returns std::strong_ordering::less if \p lhs orders before \p rhs, ::equal
///          if they are equal, and ::greater otherwise.
auto version_compare(std::string_view lhs, std::string_view rhs) -> std::strong_ordering;

/// Transparent "less" comparator built on version_compare, suitable for use as
/// the comparator of ordered associative containers keyed by filename.
struct VersionLess
{
    auto operator()(std::string_view lhs, std::string_view rhs) const -> bool
    {
        return version_compare(lhs, rhs) < 0;
    }
};

/// The single ordering policy shared by every place that merges override files
/// across config roots: an associative container keyed by basename and ordered
/// with the version-aware comparison above.
template<typename Value>
using ByBasename = std::map<std::string, Value, VersionLess>;
}

#endif // MIRAL_VERSION_COMPARE_H
