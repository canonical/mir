/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_LOCKABLE_CALLBACK_H_
#define MIR_LOCKABLE_CALLBACK_H_

namespace mir
{

class LockableCallback
{
public:
    virtual ~LockableCallback() = default;
    virtual void operator()() = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;

protected:
    LockableCallback() = default;
    LockableCallback(LockableCallback const&) = delete;
    LockableCallback& operator=(LockableCallback const&) = delete;
};

}
#endif /* MIR_LOCKABLE_CALLBACK_H_ */
