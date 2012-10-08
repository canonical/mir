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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/mir_logger.h"

#include "mir_connection.h"
#include "mir_surface.h"

#include <hardware/gralloc.h>
#include "android/android_client_buffer_factory.h"
#include "android/android_registrar_gralloc.h"

#include <cstddef>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

#ifdef MIR_USING_BOOST_THREADS
    using ::mir::std::condition_variable;
    using ::boost::unique_lock;
    using ::boost::lock_guard;
    using ::boost::thread;
    using ::boost::mutex;
    using ::mir::std::this_thread::yield;
#else
    using namespace std;
    using std::this_thread::yield;
#endif

MirConnection::MirConnection(const std::string& socket_file,
    std::shared_ptr<mcl::Logger> const & log) :
      channel(socket_file, log)
    , server(&channel)
    , log(log)
{
    {
        lock_guard<mutex> lock(connection_guard);
        valid_connections.insert(this);
    }
    connect_result.set_error("connect not called");
}

MirConnection::~MirConnection()
{
    {
        lock_guard<mutex> lock(connection_guard);
        valid_connections.erase(this);
    }
}

MirWaitHandle* MirConnection::create_surface(
    MirSurfaceParameters const & params,
    mir_surface_lifecycle_callback callback,
    void * context)
{
    const hw_module_t *hw_module;
    int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (error < 0)
        throw std::runtime_error("Could not open hardware module");

    struct alloc_device_t* alloc_dev;
    error = hw_module->methods->open(hw_module, GRALLOC_HARDWARE_GPU0, (struct hw_device_t**) &alloc_dev);
    if (error < 0)
        throw std::runtime_error("Could not open hardware module");
    gralloc_module_t* gr_dev = (gralloc_module_t*) alloc_dev;

    auto gralloc_dev = std::shared_ptr<gralloc_module_t>(gr_dev);
    auto registrar = std::make_shared<mcl::AndroidRegistrarGralloc>(gralloc_dev); 
    auto factory = std::make_shared<mcl::AndroidClientBufferFactory>(registrar); 
    auto tmp = new MirSurface(server, factory, params, callback, context);
    return tmp->get_create_wait_handle();
}

char const * MirConnection::get_error_message()
{
    if (connect_result.has_error())
    {
        return connect_result.error().c_str();
    }
    else
    {
    return error_message.c_str();
    }
}

MirWaitHandle* MirConnection::connect(
    const char* app_name,
    mir_connected_callback callback,
    void * context)
{
    connect_parameters.set_application_name(app_name);
    connect_wait_handle.result_requested();
    server.connect(
        0,
        &connect_parameters,
        &connect_result,
        google::protobuf::NewCallback(
            this, &MirConnection::connected, callback, context));
    return &connect_wait_handle;
}

void MirConnection::disconnect()
{
    disconnect_wait_handle.result_requested();
    server.disconnect(
        0,
        &ignored,
        &ignored,
        google::protobuf::NewCallback(this, &MirConnection::done_disconnect));

    disconnect_wait_handle.wait_for_result();
}

bool MirConnection::is_valid(MirConnection *connection)
{
    {
        lock_guard<mutex> lock(connection_guard);
        if (valid_connections.count(connection) == 0)
           return false;
    }

    return !connection->connect_result.has_error();
}

void MirConnection::done_disconnect()
{
    disconnect_wait_handle.result_received();
}

void MirConnection::connected(mir_connected_callback callback, void * context)
{
    auto cast = (::MirConnection*) (this);
    callback(cast, context);
    connect_wait_handle.result_received();
}

void MirConnection::populate(MirPlatformPackage& platform_package)
{
    connect_wait_handle.wait_for_result();

    if (!connect_result.has_error())
    {
        platform_package.data_items = connect_result.data_size();
        for (int i = 0; i != connect_result.data_size(); ++i)
            platform_package.data[i] = connect_result.data(i);

        platform_package.fd_items = connect_result.fd_size();
        for (int i = 0; i != connect_result.fd_size(); ++i)
            platform_package.fd[i] = connect_result.fd(i);
    }
    else
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 0;
    }
}
