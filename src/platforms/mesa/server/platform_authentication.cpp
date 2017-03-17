
#include "platform_authentication.h"
#include "mir/graphics/platform_operation_message.h"

namespace mg = mir::graphics;
namespace mgm = mg::mesa;

mgm::PlatformAuthentication::PlatformAuthentication(mgm::helpers::DRMHelper&)
{
}

mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> mgm::PlatformAuthentication::auth_extension()
{
    return {};
}

mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> mgm::PlatformAuthentication::set_gbm_extension()
{
    return {};
}

mg::PlatformOperationMessage mgm::PlatformAuthentication::platform_operation(
    unsigned int, mg::PlatformOperationMessage const&)
{
    throw std::runtime_error("");
}
