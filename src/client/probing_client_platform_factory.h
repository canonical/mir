#ifndef MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
#define MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_

#include <vector>

#include "mir/client_platform_factory.h"
#include "mir/shared_library.h"

namespace mir
{
class SharedLibraryProberReport;

namespace client
{
class ProbingClientPlatformFactory : public ClientPlatformFactory
{
public:
    ProbingClientPlatformFactory(
        std::shared_ptr<mir::SharedLibraryProberReport> const& rep);

    std::shared_ptr<ClientPlatform> create_client_platform(ClientContext *context) override;

private:
    std::shared_ptr<mir::SharedLibraryProberReport> const shared_library_prober_report;
    std::string platform_override;
};

}
}

#endif // MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
