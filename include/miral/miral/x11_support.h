/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_X11_SUPPORT_H
#define MIRAL_X11_SUPPORT_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Add a user configuration option for X11 support.
/// \remark Since MirAL 2.4
class X11Support
{
public:

    void operator()(mir::Server& server) const;

    X11Support();
    ~X11Support();
    X11Support(X11Support const&);
    auto operator=(X11Support const&) -> X11Support&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif //MIRAL_X11_SUPPORT_H
