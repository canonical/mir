#include "probing_client_platform_factory.h"
#include "mir/client_platform.h"
#include "mir/shared_library.h"
#include "mir/shared_library_prober.h"
#include "mir/shared_library_prober_report.h"

#include <boost/exception/all.hpp>
#include <stdexcept>

namespace mcl = mir::client;

mcl::ProbingClientPlatformFactory::ProbingClientPlatformFactory(
    std::shared_ptr<mir::SharedLibraryProberReport> const& rep)
    : shared_library_prober_report(rep)
{
}

std::shared_ptr<mcl::ClientPlatform>
mcl::ProbingClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    // Note we don't want to keep unused platform modules loaded any longer
    // than it takes to choose the right one. So this list is local:
    std::vector<std::shared_ptr<mir::SharedLibrary>> platform_modules;

    if (auto const platform_override = getenv("MIR_CLIENT_PLATFORM_LIB"))
    {
        // Even forcing a choice platform is only loaded on demand. It's good
        // to not need to hold the module open when there are no connections.
        // Also, this allows you to swap driver binaries on disk at run-time,
        // if you really want to.
        platform_modules.push_back(
            std::make_shared<mir::SharedLibrary>(platform_override));
    }
    else
    {
        auto const platform_path_override = getenv("MIR_CLIENT_PLATFORM_PATH");
        auto const platform_path = platform_path_override ?
                                   platform_path_override :
                                   MIR_CLIENT_PLATFORM_PATH;

        // This is sub-optimal. We shouldn't need to have all drivers loaded
        // simultaneously, but we do for now due to this API:
        platform_modules = mir::libraries_for_path(platform_path,
                                          *shared_library_prober_report);
    }

    for (auto& module : platform_modules)
    {
        try
        {
            auto probe = module->load_function<mir::client::ClientPlatformProbe>("is_appropriate_module", CLIENT_PLATFORM_VERSION);
            if (probe(context))
            {
                auto factory = module->load_function<mir::client::CreateClientPlatform>("create_client_platform", CLIENT_PLATFORM_VERSION);
                auto platform = factory(context);
                // Keep the driver module loaded only as long as we use it:
                platform->hold_resource(module);
                return platform;
            }
        }
        catch(std::runtime_error)
        {
            // We were handled a SharedLibrary that's not a client platform module?
        }
    }
    BOOST_THROW_EXCEPTION(std::runtime_error{"No appropriate client platform module found"});
}
