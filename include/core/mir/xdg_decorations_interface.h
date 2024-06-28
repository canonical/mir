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

#ifndef MIR_XDG_DECORATIONS_INTERFACE_H_
#define MIR_XDG_DECORATIONS_INTERFACE_H_

#include <memory>
namespace mir { class Server; namespace options { class Option; } }

namespace mir
{
class XdgDecorationsInterface
{
public:
    enum class Mode
    {
        always_ssd,
        always_csd,
        prefer_ssd,
        prefer_csd
    };

    virtual ~XdgDecorationsInterface() = default;

    virtual void operator()(mir::Server& server) const = 0;
    virtual auto mode() const -> Mode = 0;
};
}

#endif
