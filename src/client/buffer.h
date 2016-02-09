/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_BUFFER_H
#define MIR_CLIENT_BUFFER_H

#include "mir_toolkit/mir_buffer.h"
#include <memory>

namespace mir
{
namespace client
{
class MirBufferStub
{
public:
    MirBufferStub(MirPresentationChain* stream, mir_buffer_callback cb, void* context);
    void ready();
private:
    MirPresentationChain* const stream;
    mir_buffer_callback const cb;
    void* const context;
};
}
}
#endif /* MIR_CLIENT_BUFFER_H_ */
