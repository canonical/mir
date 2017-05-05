#ifndef MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
#define MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_

#include <vector>

#include "mir/client/client_platform_factory.h"
#include "mir/shared_library.h"

namespace mir
{
class SharedLibraryProberReport;

namespace client
{
class ProbingClientPlatformFactory : public ClientPlatformFactory
{
public:
    using StringList = std::vector<std::string>;
    ProbingClientPlatformFactory(
        std::shared_ptr<mir::SharedLibraryProberReport> const& rep,
        StringList const& force_libs,
        StringList const& lib_paths,
        std::shared_ptr<mir::logging::Logger> const& logger);

    std::shared_ptr<ClientPlatform> create_client_platform(ClientContext *context) override;

private:
    std::shared_ptr<mir::SharedLibraryProberReport> const shared_library_prober_report;
    StringList const platform_overrides;
    StringList const platform_paths;
    std::shared_ptr<mir::logging::Logger> const logger;
};

}
}

#endif // MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
