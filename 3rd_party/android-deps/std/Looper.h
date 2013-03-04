/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_LOOPER_H_
#define MIR_ANDROID_UBUNTU_LOOPER_H_

#include ANDROIDFW_UTILS(RefBase.h)

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/bind.hpp>
#include <boost/exception/all.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

// This includes some cut & paste from android/looper.h (the enums) and
// util/Looper.h (the android::Looper interface).

namespace mir_input
{
/**
 * For callback-based event loops, this is the prototype of the function
 * that is called when a file descriptor event occurs.
 * It is given the file descriptor it is associated with,
 * a bitmask of the poll events that were triggered (typically ALOOPER_EVENT_INPUT),
 * and the data pointer that was originally supplied.
 *
 * Implementations should return 1 to continue receiving callbacks, or 0
 * to have this file descriptor and callback unregistered from the looper.
 */
typedef int (*ALooper_callbackFunc)(int fd, int events, void* data);

/**
 * Flags for file descriptor events that a looper can monitor.
 *
 * These flag bits can be combined to monitor multiple events at once.
 */
enum {
    /**
     * The file descriptor is available for read operations.
     */

    ALOOPER_EVENT_INPUT = 1 << 0,

    /**
     * The file descriptor is available for write operations.
     */
    ALOOPER_EVENT_OUTPUT = 1 << 1,

    /**
     * The file descriptor has encountered an error condition.
     *
     * The looper always sends notifications about errors; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_ERROR = 1 << 2,

    /**
     * The file descriptor was hung up.
     * For example, indicates that the remote end of a pipe or socket was closed.
     *
     * The looper always sends notifications about hangups; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_HANGUP = 1 << 3,

    /**
     * The file descriptor is invalid.
     * For example, the file descriptor was closed prematurely.
     *
     * The looper always sends notifications about invalid file descriptors; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_INVALID = 1 << 4
};

enum {
    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * The poll was awoken using wake() before the timeout expired
     * and no callbacks were executed and no other file descriptors were ready.
     */
    ALOOPER_POLL_WAKE = -1,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * One or more callbacks were executed.
     */
    ALOOPER_POLL_CALLBACK = -2,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * The timeout expired.
     */
    ALOOPER_POLL_TIMEOUT = -3,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * An error occurred.
     */
    ALOOPER_POLL_ERROR = -4
};

/**
 * A polling loop that supports monitoring file descriptor events, optionally
 * using callbacks.
 *
 * A looper can be associated with a thread although there is no requirement that it must be.
 */
class Looper : public ::android::RefBase {
protected:
    virtual ~Looper() { std::lock_guard<std::mutex> lock(mutex); io_service.stop(); files.clear(); }

public:
    /**
     * Creates a looper.
     *
     * If allowNonCallbaks is true, the looper will allow file descriptors to be
     * registered without associated callbacks.  This assumes that the caller of
     * pollOnce() is prepared to handle callback-less events itself.
     */
    Looper(bool allowNonCallbacks) : woken(false)
    { if (allowNonCallbacks) BOOST_THROW_EXCEPTION(std::logic_error("Callback-less events not allowed")); }

    /**
     * Waits for events to be available, with optional timeout in milliseconds.
     * Invokes callbacks for all file descriptors on which an event occurred.
     *
     * If the timeout is zero, returns immediately without blocking.
     * If the timeout is negative, waits indefinitely until an event appears.
     *
     * Returns ALOOPER_POLL_WAKE if the poll was awoken using wake() before
     * the timeout expired and no callbacks were invoked and no other file
     * descriptors were ready.
     *
     * Returns ALOOPER_POLL_CALLBACK if one or more callbacks were invoked.
     *
     * Returns ALOOPER_POLL_TIMEOUT if there was no data before the given
     * timeout expired.
     *
     * Returns ALOOPER_POLL_ERROR if an error occurred.
     *
     * Returns a value >= 0 containing an identifier if its file descriptor has data
     * and it has no callback function (requiring the caller here to handle it).
     * In this (and only this) case outFd, outEvents and outData will contain the poll
     * events and data associated with the fd, otherwise they will be set to NULL.
     *
     * This method does not return until it has finished invoking the appropriate callbacks
     * for all file descriptors that were signalled.
     */

    inline int pollOnce(int timeoutMillis) {
        bool callbacks{false};

        // Outwit valgrind (otherwise AndroidInputManagerAndCursorListenerSetup.* test
        // runs very erratically and slowly).
        std::this_thread::yield();

        boost::system::error_code ec;

        if (timeoutMillis == 0)
        {
            callbacks |= io_service.poll(ec);
        }
        else if (timeoutMillis > 0)
        {
            boost::asio::deadline_timer timer(
                io_service,
                boost::posix_time::milliseconds(timeoutMillis));

            callbacks |= io_service.run_one(ec);
        }
        else
        {
            callbacks |= io_service.run_one(ec);
        }

        if (ec)                return ALOOPER_POLL_ERROR;
        else if (woken.load()) return ALOOPER_POLL_WAKE;
        else if (callbacks)    return ALOOPER_POLL_CALLBACK;
        else                   return ALOOPER_POLL_TIMEOUT;
    }

    /**
     * Wakes the poll asynchronously.
     *
     * This method can be called on any thread.
     * This method returns immediately.
     */
    void wake() { io_service.post([&] () {woken.store(true); }); }

    /**
     * Adds a new file descriptor to be polled by the looper.
     * If the same file descriptor was previously added, it is replaced.
     *
     * "fd" is the file descriptor to be added.
     * "ident" is an identifier for this event, which is returned from pollOnce().
     * The identifier must be >= 0, or ALOOPER_POLL_CALLBACK if providing a non-NULL callback.
     * "events" are the poll events to wake up on.  Typically this is ALOOPER_EVENT_INPUT.
     * "callback" is the function to call when there is an event on the file descriptor.
     * "data" is a private data pointer to supply to the callback.
     *
     * There are two main uses of this function:
     *
     * (1) If "callback" is non-NULL, then this function will be called when there is
     * data on the file descriptor.  It should execute any events it has pending,
     * appropriately reading from the file descriptor.  The 'ident' is ignored in this case.
     *
     * (2) If "callback" is NULL, the 'ident' will be returned by ALooper_pollOnce
     * when its file descriptor has data available, requiring the caller to take
     * care of processing it.
     *
     * Returns 1 if the file descriptor was added, 0 if the arguments were invalid.
     *
     * This method can be called on any thread.
     * This method may block briefly if it needs to wake the poll.
     *
     * The callback may either be specified as a bare function pointer or as a smart
     * pointer callback object.  The smart pointer should be preferred because it is
     * easier to avoid races when the callback is removed from a different thread.
     * See removeFd() for details.
     */
    int addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void* data)
    {   (void)ident;
        std::lock_guard<std::mutex> lock(mutex);
        files.emplace_back(io_service, fd);

        if (events & ALOOPER_EVENT_INPUT)
            boost::asio::async_read(files.back(), boost::asio::null_buffers(),
                boost::bind(callback, fd, events, data));

        return 1;
    }

    /**
     * Removes a previously added file descriptor from the looper.
     *
     * When this method returns, it is safe to close the file descriptor since the looper
     * will no longer have a reference to it.  However, it is possible for the callback to
     * already be running or for it to run one last time if the file descriptor was already
     * signalled.  Calling code is responsible for ensuring that this case is safely handled.
     * For example, if the callback takes care of removing itself during its own execution either
     * by returning 0 or by calling this method, then it can be guaranteed to not be invoked
     * again at any later time unless registered anew.
     *
     * A simple way to avoid this problem is to use the version of addFd() that takes
     * a sp<LooperCallback> instead of a bare function pointer.  The LooperCallback will
     * be released at the appropriate time by the Looper.
     *
     * Returns 1 if the file descriptor was removed, 0 if none was previously registered.
     *
     * This method can be called on any thread.
     * This method may block briefly if it needs to wake the poll.
     */
    int removeFd(int fd) {
        std::lock_guard<std::mutex> lock(mutex);
        auto erase_me = std::find_if(files.begin(), files.end(),
            [=] (boost::asio::posix::stream_descriptor& ff){ return fd == ff.native(); });

        if (erase_me == files.end()) return 0;
        files.erase(erase_me);
        return 1;
    }

private:
    std::atomic<bool> woken;
    boost::asio::io_service io_service;
    std::mutex mutable mutex;
    std::vector<boost::asio::posix::stream_descriptor> files;
};
}

namespace android
{
using ::mir_input::ALooper_callbackFunc;
using ::mir_input::ALOOPER_EVENT_INPUT;
using ::mir_input::ALOOPER_EVENT_OUTPUT;
using ::mir_input::ALOOPER_EVENT_ERROR;
using ::mir_input::ALOOPER_EVENT_HANGUP;
using ::mir_input::ALOOPER_EVENT_INVALID;
using ::mir_input::ALOOPER_POLL_WAKE;
using ::mir_input::ALOOPER_POLL_CALLBACK;
using ::mir_input::ALOOPER_POLL_TIMEOUT;
using ::mir_input::ALOOPER_POLL_ERROR;
using ::mir_input::Looper;
} // namespace android
#endif /* MIR_ANDROID_UBUNTU_LOOPER_H_ */
