/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_CLIENT_MIR_WAIT_HANDLE_H_
#define MIR_CLIENT_MIR_WAIT_HANDLE_H_

#include "mir/thread/all.h"

#ifdef MIR_USING_BOOST_THREADS
    using ::mir::std::condition_variable;
    using ::boost::unique_lock;
    using ::boost::lock_guard;
    using ::boost::thread;
    using ::boost::mutex;
    using ::mir::std::this_thread::yield;
#else
    using namespace std;
    using std::this_thread::yield;
#endif

class MirWaitHandle
{
public:
    MirWaitHandle();
    ~MirWaitHandle();

    void result_received();
    void wait_for_result();

private:
    mutex guard;
    condition_variable wait_condition;

    bool result_has_occurred;
};

#endif /* MIR_CLIENT_MIR_WAIT_HANDLE_H_ */
