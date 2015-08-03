/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
*/

#ifndef MIR_TEST_WAIT_OBJECT_H_
#define MIR_TEST_WAIT_OBJECT_H_

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <boost/throw_exception.hpp>

namespace mir
{
namespace test
{

class WaitObject
{
public:
    void notify_ready();
    void wait_until_ready();
    template<typename Representation, typename Period>
    void wait_until_ready(std::chrono::duration<Representation, Period> const& span)
    {
        std::unique_lock<std::mutex> lock{mutex};
        if (!cv.wait_for(lock, span, [this] { return ready; }))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("WaitObject timed out"));
        }
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
};

}
}

#endif

