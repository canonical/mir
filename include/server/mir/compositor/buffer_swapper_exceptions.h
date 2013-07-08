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

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_EXCEPTIONS_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_EXCEPTIONS_H_

#include <stdexcept>

namespace mir
{
namespace compositor
{

struct BufferSwapperRequestAbortedException : public std::runtime_error
{
    BufferSwapperRequestAbortedException() : std::runtime_error("Request aborted") {}
};

struct BufferSwapperOutOfBuffersException : public std::logic_error
{
    BufferSwapperOutOfBuffersException() : std::logic_error("Out of buffers") {}
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_EXCEPTIONS_H_ */
