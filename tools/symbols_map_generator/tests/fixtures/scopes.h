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

// Namespacing, enums, inline functions and variables.  Nested namespaces
// produce a "::"-joined prefix.  Enums (scoped or unscoped) emit no
// symbols.  An inline (in-header defined) free function is skipped, while
// a declared-only free function and an extern variable are emitted.

namespace mir
{
namespace nested
{

enum class ScopedEnum
{
    First,
    Second
};

enum UnscopedEnum
{
    Alpha,
    Beta
};

inline void inline_free_function()
{
}

void declared_free_function();

extern int extern_variable;

}
}
