/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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

#ifndef MIR_CLIENT_COOKIE_H
#define MIR_CLIENT_COOKIE_H

#include <mir_toolkit/mir_cookie.h>

#include <memory>

namespace mir
{
namespace client
{
class Cookie
{
public:
    Cookie() = default;
    explicit Cookie(MirCookie const* cookie) : self{cookie, deleter} {}

    operator MirCookie const*() const { return self.get(); }

    void reset() { self.reset(); }
    void reset(MirCookie const* cookie) { self.reset(cookie, deleter); }

    friend void mir_cookie_release(Cookie const&) = delete;

private:
    static void deleter(MirCookie const* cookie) { mir_cookie_release(cookie); }
    std::shared_ptr<MirCookie const> self;
};
}
}

#endif //MIR_CLIENT_COOKIE_H
