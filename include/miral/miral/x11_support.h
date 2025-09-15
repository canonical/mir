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

#ifndef MIRAL_X11_SUPPORT_H
#define MIRAL_X11_SUPPORT_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Add user configuration options for X11 support.
/// \note From MirAL 4.1, the \c --enable-x11 option requires an argument
///       (e.g., \c --enable-x11=true to enable X11 support).
class X11Support
{
public:

    void operator()(mir::Server& server) const;

    X11Support();
    ~X11Support();
    X11Support(X11Support const&);
    auto operator=(X11Support const&) -> X11Support&;

    /**
     * Set X11 support as enabled by default.
     *
     * \return A reference to the modified miral::X11Support object with
     *         the updated default configuration.
     * \remark Since MirAL 4.1
     */
    auto default_to_enabled() -> X11Support&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif //MIRAL_X11_SUPPORT_H
