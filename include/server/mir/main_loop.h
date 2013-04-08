/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_MAIN_LOOP_H_
#define MIR_MAIN_LOOP_H_

#include <functional>
#include <initializer_list>

namespace mir
{

class MainLoop
{
public:
    virtual ~MainLoop() = default;

    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler) = 0;

protected:
    MainLoop() = default;
    MainLoop(MainLoop const&) = delete;
    MainLoop& operator=(MainLoop const&) = delete;
};

}

#endif /* MIR_MAIN_LOOP_H_ */
