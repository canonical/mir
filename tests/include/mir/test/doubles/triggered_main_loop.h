/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_TRIGGERED_MAIN_LOOP_H_
#define MIR_TEST_DOUBLES_TRIGGERED_MAIN_LOOP_H_

#include "mir/test/doubles/mock_main_loop.h"

#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{

class TriggeredMainLoop : public ::testing::NiceMock<MockMainLoop>
{
public:
    using fd_callback = std::function<void(int)>;
    using signal_callback = std::function<void(int)>;
    using callback = std::function<void()>;

    void register_fd_handler(std::initializer_list<int> fds, void const* owner, fd_callback const& handler) override;
    void unregister_fd_handler(void const* owner) override;
    std::unique_ptr<mir::time::Alarm> create_alarm(callback const& call) override;
    void enqueue(void const* owner, ServerAction const& action) override;
    void enqueue_with_guaranteed_execution(ServerAction const& action) override;

    void spawn(std::function<void()>&& work) override;

    void trigger_pending_fds();
    void fire_all_alarms();
    void trigger_server_actions();
    void trigger_spawned_work();

private:
    std::vector<callback> timeout_callbacks;

    struct Item
    {
        int fd;
        void const* owner;
        fd_callback callback;
    };
    std::vector<Item> fd_callbacks;
    std::vector<ServerAction> actions;
    std::vector<std::function<void()>> work;
};

}
}
}

#endif
