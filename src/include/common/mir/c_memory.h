/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_C_MEMORY_H
#define MIR_C_MEMORY_H

#include <memory>
#include <stdlib.h>

namespace mir
{
struct DeleteCPtr { template<typename T> void operator()(T* p) { ::free(p); }};

template<typename T> using UniqueCPtr = std::unique_ptr<T, DeleteCPtr>;
template<typename T> auto make_unique_cptr(T* p) { return UniqueCPtr<T>{p}; }
}

#endif  // MIR_C_MEMORY_H
