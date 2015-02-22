#include "probing_client_platform_factory.h"

#include <boost/exception/all.hpp>
#include <stdexcept>

namespace mcl = mir::client;

mcl::ProbingClientPlatformFactory::ProbingClientPlatformFactory(std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules)
    : platform_modules{modules}
{
    if (modules.empty())
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error{"Attempted to create a ClientPlatformFactory with no platform modules"});
    }
}

std::shared_ptr<mcl::ClientPlatform>
mcl::ProbingClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    for (auto& module : platform_modules)
    {
        try
        {
            auto probe = module->load_function<mir::client::ClientPlatformProbe>("is_appropriate_module", CLIENT_PLATFORM_VERSION);
            if (probe(context))
            {
                auto factory = module->load_function<mir::client::CreateClientPlatform>("create_client_platform", CLIENT_PLATFORM_VERSION);
                return factory(context);
            }
        }
        catch(std::runtime_error)
        {
            // We were handled a SharedLibrary that's not a client platform module?
        }
    }
    BOOST_THROW_EXCEPTION(std::runtime_error{"No appropriate client platform module found"});
}
