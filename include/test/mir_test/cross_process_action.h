/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_CROSS_PROCESS_ACTION_H_
#define MIR_TEST_CROSS_PROCESS_ACTION_H_

/* TODO: CrossProcessSync should be in mir_test */
#include "mir_test_framework/cross_process_sync.h"

#include <functional>

namespace mir
{
namespace test
{

class CrossProcessAction
{
public:
    void exec(std::function<void()> const& f);
    void operator()();

private:
    mir_test_framework::CrossProcessSync start_sync;
    mir_test_framework::CrossProcessSync finish_sync;
};

}
}

#endif /* MIR_TEST_CROSS_PROCESS_ACTION_H_ */
