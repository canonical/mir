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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_

#include "client_buffer.h"
#include "mir_client/mir_client_library.h"
#include "android/android_registrar.h"

#include <memory>

namespace mir
{
namespace client
{

class AndroidClientBuffer : public ClientBuffer
{
public:
    AndroidClientBuffer(std::shared_ptr<AndroidRegistrar>, const std::shared_ptr<MirBufferPackage> ,
                        geometry::Width width, geometry::Height height, geometry::PixelFormat pf );
    ~AndroidClientBuffer();
    
    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Width width() const;
    geometry::Height height() const;
    geometry::PixelFormat pixel_format() const;

    AndroidClientBuffer(const AndroidClientBuffer&) = delete;
    AndroidClientBuffer& operator=(const AndroidClientBuffer&) = delete;
private:
    const native_handle_t* convert_to_native_handle(const std::shared_ptr<MirBufferPackage>& package);

    std::shared_ptr<const native_handle_t> native_handle;
    std::shared_ptr<AndroidRegistrar> buffer_registrar;

    const geometry::Rectangle rect;
    const geometry::PixelFormat buffer_pf;
};

}
}
#endif /* MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_ */
