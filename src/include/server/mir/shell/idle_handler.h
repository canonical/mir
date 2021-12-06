/*
 * Copyright © 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_IDLE_HANDLER_H_
#define MIR_SHELL_IDLE_HANDLER_H_

namespace mir
{
namespace shell
{

class IdleHandler
{
public:
    IdleHandler() = default;
    virtual ~IdleHandler() = default;

private:
    IdleHandler(IdleHandler const&) = delete;
    IdleHandler& operator=(IdleHandler const&) = delete;
};

}
}

#endif // MIR_SHELL_IDLE_HANDLER_H_
