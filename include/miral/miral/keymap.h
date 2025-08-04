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

#ifndef MIRAL_KEYMAP_H
#define MIRAL_KEYMAP_H

#include <memory>
#include <string>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Load a keymap
class Keymap
{
public:

    /// Apply keymap from the config.
    Keymap();

    /// Specify a keymap.
    /// Format is:
    /// \verbatim <language>[+<variant>[+<options>]] \endverbatim
    /// Options is a comma separated list.
    /// e.g. "gb" or "us+dvorak"
    explicit Keymap(std::string const& keymap);

    /// Construct a `Keymap` instance with access to a live config store.
    /// \remark Since MirAL 5.5
    explicit Keymap(live_config::Store& config_store);

    /// Construct a `Keymap` instance that tracks the system locale1 dbus node
    /// \remark Since MirAL 5.6
    static auto system_locale1() -> Keymap;

    ~Keymap();
    Keymap(Keymap const& that);
    auto operator=(Keymap const& rhs) -> Keymap&;

    void operator()(mir::Server& server) const;

    /// Specify a new keymap.
    void set_keymap(std::string const& keymap);

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_KEYMAP_H
