/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_BASIC_CALLBACK_H_
#define MIR_BASIC_CALLBACK_H_

#include "mir/lockable_callback.h"

#include <functional>

namespace mir
{

class BasicCallback : public mir::LockableCallback
{
public:
    BasicCallback(std::function<void()> const& callback);

    void operator()() override;
    void lock() override;
    void unlock() override;

private:
    std::function<void()> callback;
};

}

#endif
