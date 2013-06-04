#include <cassert>
#include <cstring>
#include <thread>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "client.h"
#include "mir_toolkit/mir_client_library.h"

ClientStateMachine::Ptr ClientStateMachine::Create()
{
    // in the future we can extend this to return different
    // types of clients.
    return std::make_shared<UnacceleratedClient>();
}

UnacceleratedClient::UnacceleratedClient()
: connection_(nullptr)
, surface_(nullptr)
{
}

bool UnacceleratedClient::connect(std::string unique_name, const char* socket_file)
{
    assert(connection_ == nullptr);

    connection_ = mir_connect_sync(socket_file, unique_name.c_str());

    if (! connection_ || ! mir_connection_is_valid(connection_))
    {
        const char* error_message = mir_connection_get_error_message(connection_);
        if (std::strcmp(error_message, "") != 0)
        {
            std::cout << "Could not connect to server, error is: " << error_message << std::endl;
        }
        return false;
    }
    return true;
}

bool UnacceleratedClient::create_surface()
{
    MirDisplayInfo display_info;
    mir_connection_get_display_info(connection_, &display_info);
    if (display_info.supported_pixel_format_items < 1)
        return false;

    // TODO: instead of picking the first pixel format, pick a random one!
    MirPixelFormat const pixel_format = display_info.supported_pixel_format[0];
    MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            640,
            480,
            pixel_format,
            mir_buffer_usage_software
        };
    surface_ = mir_connection_create_surface_sync(connection_, &request_params);
    return true;
}

bool UnacceleratedClient::render_surface()
{
    return true;
}

void UnacceleratedClient::release_surface()
{
    if (surface_ != nullptr)
    {
        mir_surface_release_sync(surface_);
        surface_ = nullptr;
    }
}

void UnacceleratedClient::disconnect()
{
    if (connection_ != nullptr)
    {
        MirPlatformPackage pkg;
        mir_connection_get_platform(connection_, &pkg);
        close(pkg.fd[0]);
        mir_connection_release(connection_);
        connection_ = nullptr;
    }
}
