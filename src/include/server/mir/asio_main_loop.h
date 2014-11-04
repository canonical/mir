/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_ASIO_MAIN_LOOP_H_
#define MIR_ASIO_MAIN_LOOP_H_

#include "mir/main_loop.h"

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <utility>
#include <deque>
#include <set>

namespace mir
{

namespace time
{
class Clock;
}

class AsioMainLoop : public MainLoop
{
public:
    explicit AsioMainLoop(std::shared_ptr<time::Clock> const& clock);
    ~AsioMainLoop() noexcept(true);

    void run() override;
    void stop() override;

    void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler) override;

    void register_fd_handler(
        std::initializer_list<int> fd,
        void const* owner,
        std::function<void(int)> const& handler) override;

    void unregister_fd_handler(void const* owner) override;

    std::unique_ptr<time::Alarm> notify_in(std::chrono::milliseconds delay,
                                           std::function<void()> callback) override;
    std::unique_ptr<time::Alarm> notify_at(mir::time::Timestamp time_point,
                                           std::function<void()> callback) override;
    std::unique_ptr<time::Alarm> create_alarm(std::function<void()> callback) override;

    void enqueue(void const* owner, ServerAction const& action) override;
    void pause_processing_for(void const* owner) override;
    void resume_processing_for(void const* owner) override;

private:
    class SignalHandler;
    class FDHandler;
    bool should_process(void const*);
    void process_server_actions();

    boost::asio::io_service io;
    boost::asio::io_service::work work;
    boost::optional<std::thread::id> main_loop_thread;
    std::vector<std::unique_ptr<SignalHandler>> signal_handlers;
    std::vector<std::shared_ptr<FDHandler>> fd_handlers;
    std::mutex fd_handlers_mutex;
    std::mutex server_actions_mutex;
    std::deque<std::pair<void const*,ServerAction>> server_actions;
    std::set<void const*> do_not_process;
    std::shared_ptr<time::Clock> const clock;
};

}

#endif /* MIR_ASIO_MAIN_LOOP_H */
