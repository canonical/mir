#include "mir_client/mir_client_library.h"
#include "mir_connection.h"
#include "client_buffer_factory.h"
#include "client_buffer.h"

namespace mcl=mir::client;
namespace geom=mir::geometry;

class EmptyBuffer : public mcl::ClientBuffer
{
    std::shared_ptr<mcl::MemoryRegion> secure_for_cpu_write()
    {
        return std::make_shared<mcl::MemoryRegion>();
    };
    geom::Width width() const
    {
        return geom::Width{0};
    }
    geom::Height height() const
    {
        return geom::Height{0};
    }
    geom::PixelFormat pixel_format() const
    {
        return geom::PixelFormat::rgba_8888;
    }
};

/* todo: replace with real buffer factory */
class EmptyFactory : public mcl::ClientBufferFactory
{
public:
    std::shared_ptr<mcl::ClientBuffer> create_buffer_from_ipc_message(const std::shared_ptr<MirBufferPackage>&,
                                geom::Width, geom::Height, geom::PixelFormat) 
    {
        return std::make_shared<EmptyBuffer>();
    }
};

std::shared_ptr<mcl::ClientBufferFactory> mcl::create_platform_factory()
{
    return std::make_shared<EmptyFactory>();
}
