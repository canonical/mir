#ifndef MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
#define MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_

#include <vector>

#include "client_platform_factory.h"
#include "mir/shared_library.h"

namespace mir
{
namespace client
{
class ProbingClientPlatformFactory : public ClientPlatformFactory
{
public:
    ProbingClientPlatformFactory(std::vector<std::shared_ptr<SharedLibrary>> const& modules);

    std::shared_ptr<ClientPlatform> create_client_platform(ClientContext *context) override;
private:
    std::vector<std::shared_ptr<SharedLibrary>> platform_modules;
};

}
}

#endif // MIR_CLIENT_PROBING_CLIENT_PLATFORM_FACTORY_H_
