
#include "android/android_registrar_gralloc.h"
#include "android/android_client_buffer_factory.h"
#include "mir_connection.h"

namespace mcl=mir::client;

std::shared_ptr<mcl::ClientBufferFactory> mcl::create_platform_factory()
{
    const hw_module_t *hw_module;
    int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (error < 0)
    {
        throw std::runtime_error("Could not open hardware module");
    }

    gralloc_module_t* gr_dev = (gralloc_module_t*) hw_module;
    auto gralloc_dev = std::shared_ptr<gralloc_module_t>(gr_dev);
    auto registrar = std::make_shared<mcl::AndroidRegistrarGralloc>(gralloc_dev); 
    return std::make_shared<mcl::AndroidClientBufferFactory>(registrar);
}
