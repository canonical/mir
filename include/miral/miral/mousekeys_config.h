/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_ENABLE_MOUSEKEYS_H
#define MIRAL_ENABLE_MOUSEKEYS_H

#include <memory>

namespace mir
{
class Server;
namespace input
{
class MouseKeysKeymap;
}
}

/// Enables configuring mousekeys at runtime.
/// \remark Since MirAL 5.3
namespace miral
{
class MouseKeysConfig
{
public:
    /// \note `--enable-mouse-keys` has higher precedence than
    /// [enabled_by_default]
    explicit MouseKeysConfig(bool enabled_by_default);
    void operator()(mir::Server& server) const;

    /// Toggles mousekeys on or off depending on the passed parameter.
    /// \note Can only be called after the server is initialized.
    void toggle_mousekeys(bool enabled) const;

    /// Changes the keymap for various mousekeys actions
    /// \note Can only be called after the server is initialized.
    /// \note The keymap _must_ cover all actions found in [MouseKeysActions].
    void set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
