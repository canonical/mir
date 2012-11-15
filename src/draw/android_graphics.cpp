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


#include "mir/draw/android_graphics.h"
#include "mir/compositor/buffer_ipc_package.h"

#include <stdexcept>
namespace mt=mir::draw;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

mt::grallocRenderSW::grallocRenderSW()
{
    const hw_module_t *hw_module;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module) != 0)
        throw std::runtime_error("error, hw module not available!\n");
    gralloc_open(hw_module, &alloc_dev);
    module = (gralloc_module_t*) hw_module;
}

mt::grallocRenderSW::~grallocRenderSW()
{
    gralloc_close(alloc_dev);
}

void mt::grallocRenderSW::render_pattern(std::shared_ptr<mc::GraphicBufferClientResource> res, geom::Size size, int val)
{
    auto ipc_pack = res->ipc_package;

    int width =  size.width.as_uint32_t(); 
    int height = size.height.as_uint32_t();

    /* reconstruct the native_window_t */
    native_handle_t* native_handle;
    int num_fd = ipc_pack->ipc_fds.size(); 
    int num_data = ipc_pack->ipc_data.size(); 
    native_handle = (native_handle_t*) malloc(sizeof(int) * (3+num_fd+num_data));
    native_handle->numFds = num_fd;
    native_handle->numInts = num_data;

    int i=0;
    for(auto it=ipc_pack->ipc_fds.begin(); it != ipc_pack->ipc_fds.end(); it++)
    {
        native_handle->data[i++] = *it;
    }

    i=num_fd;
    for(auto it=ipc_pack->ipc_data.begin(); it != ipc_pack->ipc_data.end(); it++)
    {
        native_handle->data[i++] = *it;
    }
    

    int *buffer_vaddr;
   
    int ret; 
    ret = module->lock(module, native_handle, GRALLOC_USAGE_SW_WRITE_OFTEN,
                0, 0, width, height, (void**) &buffer_vaddr);
    if (!ret)
        return;

    int j;
    for(i=0; i<width; i++)
    {
        for(j=0; j<height; j++)
        {        
            buffer_vaddr[width*i + j] = val;
        }
    }
    module->unlock(module, native_handle);

}
