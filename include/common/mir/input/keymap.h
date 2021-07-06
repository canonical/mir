/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_KEYMAP_H_
#define MIR_INPUT_KEYMAP_H_

#include <string>
#include <ostream>

namespace mir
{
namespace input
{

struct Keymap
{
    Keymap() = default;
    Keymap(std::string&& model,
           std::string&& layout,
           std::string&& variant,
           std::string&& options)
        : model{model}, layout{layout}, variant{variant}, options{options}
    {
    }

    Keymap(std::string const& model,
           std::string const& layout,
           std::string const& variant,
           std::string const& options)
        : model{model}, layout{layout}, variant{variant}, options{options}
    {
    }

    std::string model{"pc105+inet"};
    std::string layout{"us"};
    std::string variant;
    std::string options;

};

inline bool operator==(Keymap const& lhs, Keymap const& rhs)
{
    return lhs.model == rhs.model &&
        lhs.layout == rhs.layout &&
        lhs.variant == rhs.variant &&
        lhs.options == rhs.options;
}

inline bool operator!=(Keymap const& lhs, Keymap const& rhs)
{
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream &out, Keymap const& rhs)
{
    return out << rhs.model << "-" << rhs.layout << "-"<< rhs.variant << "-" << rhs.options;
}

}
}

#endif

