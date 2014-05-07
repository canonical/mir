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

#include "mir/asio_main_loop.h"

#include <cassert>

class mir::AsioMainLoop::SignalHandler
{
public:
    SignalHandler(boost::asio::io_service& io,
                  std::initializer_list<int> signals,
                  std::function<void(int)> const& handler)
        : signal_set{io},
          handler{handler}
    {
        for (auto sig : signals)
            signal_set.add(sig);
    }

    void async_wait()
    {
        signal_set.async_wait(
            std::bind(&SignalHandler::handle, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

private:
    void handle(boost::system::error_code err, int sig)
    {
        if (!err)
        {
            handler(sig);
            signal_set.async_wait(
                std::bind(&SignalHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2));
        }
    }

    boost::asio::signal_set signal_set;
    std::function<void(int)> handler;
};

class mir::AsioMainLoop::FDHandler
{
public:
    FDHandler(boost::asio::io_service& io,
              std::initializer_list<int> fds,
              std::function<void(int)> const& handler)
        : handler{handler}
    {
        for (auto fd : fds)
        {
            auto raw = new boost::asio::posix::stream_descriptor{io, fd};
            auto s = std::unique_ptr<boost::asio::posix::stream_descriptor>(raw);
            stream_descriptors.push_back(std::move(s));
        }
    }

    void async_wait()
    {
        for (auto const& s : stream_descriptors)
        {
            s->async_read_some(
                boost::asio::null_buffers(),
                std::bind(&FDHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2, s.get()));
        }
    }

private:
    void handle(boost::system::error_code err, size_t /*bytes*/,
                boost::asio::posix::stream_descriptor* s)
    {
        if (!err)
        {
            handler(s->native_handle());
            s->async_read_some(
                boost::asio::null_buffers(),
                std::bind(&FDHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2, s));
        }
    }

    std::vector<std::unique_ptr<boost::asio::posix::stream_descriptor>> stream_descriptors;
    std::function<void(int)> handler;
};

/*
 * We need to define an empty constructor and destructor in the .cpp file,
 * so that we can use unique_ptr to hold SignalHandler. Otherwise, users
 * of AsioMainLoop end up creating default constructors and destructors
 * that don't have complete type information for SignalHandler and fail
 * to compile.
 */
mir::AsioMainLoop::AsioMainLoop()
{
}

mir::AsioMainLoop::~AsioMainLoop() noexcept(true)
{
}

void mir::AsioMainLoop::run()
{
    io.run();
}

void mir::AsioMainLoop::stop()
{
    io.stop();
}

void mir::AsioMainLoop::register_signal_handler(
    std::initializer_list<int> signals,
    std::function<void(int)> const& handler)
{
    assert(handler);

    auto sig_handler = std::unique_ptr<SignalHandler>{
        new SignalHandler{io, signals, handler}};

    sig_handler->async_wait();

    signal_handlers.push_back(std::move(sig_handler));
}

void mir::AsioMainLoop::register_fd_handler(
    std::initializer_list<int> fds,
    std::function<void(int)> const& handler)
{
    assert(handler);

    auto fd_handler = std::unique_ptr<FDHandler>{
        new FDHandler{io, fds, handler}};

    fd_handler->async_wait();

    fd_handlers.push_back(std::move(fd_handler));
}

void mir::AsioMainLoop::post(ServerActionType type, ServerAction const& action)
{
    {
        std::lock_guard<std::mutex> lock{server_actions_mutex};
        server_actions.push_back({type, action});
    }

    io.post([this] { process_server_actions(); });
}

void mir::AsioMainLoop::stop_processing(ServerActionType type)
{
    std::lock_guard<std::mutex> lock{server_actions_mutex};
    do_not_process.insert(type);
}

void mir::AsioMainLoop::resume_processing(ServerActionType type)
{
    {
        std::lock_guard<std::mutex> lock{server_actions_mutex};
        do_not_process.erase(type);
    }

    io.post([this] { process_server_actions(); });
}

void mir::AsioMainLoop::process_server_actions()
{
    std::unique_lock<std::mutex> lock{server_actions_mutex};

    size_t i = 0;

    while (i < server_actions.size())
    {
        /* 
         * It's safe to use references to elements, since std::deque<>
         * guarantees that references remain valid after appends, which is
         * the only operation that can be performed on server_actions outside
         * this function (in AsioMainLoop::post()).
         */
        auto const& type = server_actions[i].first;
        auto const& action = server_actions[i].second;

        if (do_not_process.find(type) == do_not_process.end())
        {
            lock.unlock();
            action();
            lock.lock();
            /*
             * This erase is always ok, since outside this function
             * we only append to server_actions, i.e., our index i
             * is guaranteed to remain valid and correct.
             */
            server_actions.erase(server_actions.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}
