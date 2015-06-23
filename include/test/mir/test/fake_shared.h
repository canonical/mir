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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FAKE_SHARED_H_
#define MIR_TEST_FAKE_SHARED_H_

#include "mir/test/empty_deleter.h"

#include <memory>

namespace mir
{
namespace test
{
template <typename Type>
std::shared_ptr<Type> fake_shared(Type& t) { return {&t, EmptyDeleter()}; }
}
}

#endif /* MIR_TEST_FAKE_SHARED_H_ */
