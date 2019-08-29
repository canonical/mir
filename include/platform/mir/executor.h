/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_EXECUTOR_H_
#define MIR_EXECUTOR_H_

#include <functional>

namespace mir
{

/**
 * An executor abstraction mostly compatible with C++ proposal N4414
 *
 * As specified in http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0008r0.pdf
 *
 * This should hopefully be compatible with whatever the C++ Executors and Schedulers group
 * come up with as a final design, and will eventually be replaced by the standard version.
 *
 * If not, this minimal interface should be easy to implement on top of whatever emerges
 * from the standards body.
 */
class Executor
{
public:
    /**
     * Schedule some function to be called sometime in the future.
     *
     * It is expected that this \b not eagerly execute, and will return
     * \b before work() is executed.
     *
     * \param [in] work     Function to execute in Executor specified environment.
     */
    virtual void spawn(std::function<void()>&& work) = 0;

    virtual ~Executor() = default;
};

}

#endif // MIR_EXECUTOR_H_
