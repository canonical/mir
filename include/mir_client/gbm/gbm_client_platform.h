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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_GBM_GBM_CLIENT_PLATFORM_H_
#define MIR_CLIENT_GBM_GBM_CLIENT_PLATFORM_H_

#include "mir_client/client_platform.h"

namespace mir
{
namespace client
{
class ClientBufferDepository;

namespace gbm
{

class DRMFDHandler;

class GBMClientPlatform : public ClientPlatform
{
public:
    GBMClientPlatform(ClientConnection* const connection,
                      std::shared_ptr<DRMFDHandler> const& drm_fd_handler);

    std::shared_ptr<ClientBufferDepository> create_platform_depository ();
    EGLNativeWindowType create_egl_window(ClientSurface *surface);
    void destroy_egl_window(EGLNativeWindowType window);
    std::shared_ptr<EGLNativeDisplayContainer> create_egl_native_display();

private:
    ClientConnection* const connection;
    std::shared_ptr<DRMFDHandler> const drm_fd_handler;
};

}
}
}

#endif /* MIR_CLIENT_GBM_GBM_CLIENT_PLATFORM_H_ */
