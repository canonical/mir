/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_STREAM_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_STREAM_FACTORY_H_

#include <mir/scene/buffer_stream_factory.h>
#include <mir_test_doubles/stub_buffer.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBufferStreamFactory : public scene::BufferStreamFactory
{
public:
    std::shared_ptr<compositor::BufferStream> create_buffer_stream(
        int, graphics::BufferProperties const&) { return nullptr; }
    std::shared_ptr<compositor::BufferStream> create_buffer_stream(
        graphics::BufferProperties const&) { return nullptr; }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_BUFFER_STREAM_FACTORY_H_ */
