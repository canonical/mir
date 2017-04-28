/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_CHAIN_H_
#define MIR_GRAPHICS_NESTED_HOST_CHAIN_H_

namespace mir
{
namespace graphics
{
namespace nested
{
class NativeBuffer;

enum class SubmissionMode
{
    queueing,
    dropping
};
class HostChain
{
public:
    virtual ~HostChain() = default;
    virtual void submit_buffer(NativeBuffer&) = 0;
    virtual void set_submission_mode(SubmissionMode mode) = 0;
    virtual MirPresentationChain* handle() = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    virtual MirRenderSurface* rs() const = 0;
#pragma GCC diagnostic pop
protected:
    HostChain() = default;
    HostChain(HostChain const&) = delete;
    HostChain& operator=(HostChain const&) = delete;
};
}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_CHAIN_H_
