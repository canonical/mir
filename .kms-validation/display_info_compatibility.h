#include <cstdint>
#include <stdexcept>

struct KMSValidationHDMIBlock
{
    uint16_t source_phys_addr;
};

inline KMSValidationHDMIBlock const* di_cta_data_block_get_vendor_hdmi(void const*)
{
    throw std::runtime_error{"The host libdisplay-info does not provide HDMI vendor block parsing"};
}
