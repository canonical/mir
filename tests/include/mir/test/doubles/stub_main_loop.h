/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_STUB_MAIN_LOOP_H
#define MIR_TEST_DOUBLES_STUB_MAIN_LOOP_H

#include "mir/main_loop.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubMainLoop : public MainLoop
{
public:
    void run() override {}
    void stop() override {}
    bool running() const override { return false; }

    void register_signal_handler(
        std::initializer_list<int>,
        std::function<void(int)> const&) override {}

    void register_signal_handler(
        std::initializer_list<int>,
        mir::UniqueModulePtr<std::function<void(int)>>) override {}

    void register_fd_handler(
        std::initializer_list<int>,
        void const*,
        std::function<void(int)> const&) override {}

    void register_fd_handler(
        std::initializer_list<int>,
        void const*,
        mir::UniqueModulePtr<std::function<void(int)>>) override {}

    void unregister_fd_handler(void const*) override {}

    std::unique_ptr<time::Alarm> create_alarm(std::function<void()> const&) override
    {
        return nullptr;
    }

    std::unique_ptr<time::Alarm> create_alarm(std::unique_ptr<LockableCallback>) override
    {
        return nullptr;
    }

    void enqueue(void const*, ServerAction const&) override {}

    void enqueue_with_guaranteed_execution(ServerAction const&) override {}

    void pause_processing_for(void const*) override {}

    void resume_processing_for(void const*) override {}

    void spawn(std::function<void()>&&) override {}
};

}
}
}

#endif //MIR_TEST_DOUBLES_STUB_MAIN_LOOP_H
