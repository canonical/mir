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

namespace mir { class Server; }

/// Enable toggling mousekeys via a keyboard shortcut (Shift+Ctrl+Numlock) at
/// runtime.
/// \remark Since MirAL 5.3
namespace miral
{
class EnableMouseKeys
{
public:
    EnableMouseKeys();
    void operator()(mir::Server& server) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
