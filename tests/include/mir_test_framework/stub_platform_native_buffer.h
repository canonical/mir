
#ifndef MIR_TEST_FRAMEWORK_STUB_PLATFORM_NATIVE_BUFFER_H_
#define MIR_TEST_FRAMEWORK_STUB_PLATFORM_NATIVE_BUFFER_H_

#include <mir/graphics/buffer_properties.h>
#include <mir/graphics/native_buffer.h>
#include <mir/fd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mir_test_framework
{
//just a simple FD and int, helps to check for leaks/memory issues.
struct NativeBuffer : mir::graphics::NativeBuffer
{
    NativeBuffer(mir::graphics::BufferProperties const& properties) : 
        properties(properties)
    {
        if (fd < 0)
        {
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::system_error(errno, std::system_category(), "Failed to open dummy fd")));
        }
    }
    int const data {0x328};
    mir::Fd const fd{open("/dev/zero", O_RDONLY)};
    mir::graphics::BufferProperties const properties;
};
}
#endif /* MIR_TEST_FRAMEWORK_STUB_PLATFORM_NATIVE_BUFFER_H_ */
