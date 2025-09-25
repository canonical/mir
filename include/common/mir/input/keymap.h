/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_KEYMAP_H_
#define MIR_INPUT_KEYMAP_H_

#include <memory>
#include <string>

struct xkb_keymap;
struct xkb_context;

namespace mir
{
namespace input
{
using XKBKeymapPtr = std::unique_ptr<xkb_keymap, void(*)(xkb_keymap*)>;

class Keymap
{
public:
    Keymap() = default;
    virtual ~Keymap() = default;

    /// Always returns false if this will produce a different XKB keymap than other
    virtual auto matches(Keymap const& other) const -> bool = 0;
    /// The model name of the keyboard this keymap is for
    virtual auto model() const -> std::string = 0;
    virtual auto make_unique_xkb_keymap(xkb_context* context) const -> XKBKeymapPtr = 0;

private:
    Keymap(Keymap const&) = delete;
    Keymap& operator=(Keymap const&) = delete;
};

}
}

#endif
