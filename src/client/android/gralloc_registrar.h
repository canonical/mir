/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_GRALLOC_REGISTRAR_H_
#define MIR_CLIENT_ANDROID_GRALLOC_REGISTRAR_H_

#include "mir_toolkit/common.h"
#include "buffer_registrar.h"
#include <hardware/gralloc.h>

namespace mir
{
namespace client
{
namespace android
{

class GrallocRegistrar : public BufferRegistrar
{
public:
    GrallocRegistrar(const std::shared_ptr<const gralloc_module_t>& gralloc_dev);

    std::shared_ptr<const native_handle_t> register_buffer(
        std::shared_ptr<MirBufferPackage> const& package) const;
    std::shared_ptr<char> secure_for_cpu(std::shared_ptr<const native_handle_t> handle, const geometry::Rectangle);

private:
    std::shared_ptr<const gralloc_module_t> gralloc_module;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_GRALLOC_REGISTRAR_H_ */
