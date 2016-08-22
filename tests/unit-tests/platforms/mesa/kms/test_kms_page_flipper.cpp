/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "src/platforms/mesa/server/kms/kms_page_flipper.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_display_report.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <atomic>
#include <thread>
#include <unordered_set>

#include <sys/time.h>

namespace mg  = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt  = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

class KMSPageFlipperTest : public ::testing::Test
{
public:
    KMSPageFlipperTest()
        : page_flipper{mock_drm.fake_drm.fd(), mt::fake_shared(report)}
    {
    }

    testing::NiceMock<mtd::MockDisplayReport> report;
    testing::NiceMock<mtd::MockDRM> mock_drm;
    mgm::KMSPageFlipper page_flipper;
};

ACTION_P(InvokePageFlipHandler, param)
{
    int const dont_care{0};
    char dummy;

    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, *param);
    ASSERT_EQ(1, read(arg0, &dummy, 1));
}

}

TEST_F(KMSPageFlipperTest, schedule_flip_calls_drm_page_flip)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    page_flipper.schedule_flip(crtc_id, fb_id);
}

TEST_F(KMSPageFlipperTest, double_schedule_flip_throws)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    page_flipper.schedule_flip(crtc_id, fb_id);

    EXPECT_THROW({
        page_flipper.schedule_flip(crtc_id, fb_id);
    }, std::logic_error);
}

TEST_F(KMSPageFlipperTest, wait_for_flip_handles_drm_event)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};
    void* user_data{nullptr};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<4>(&user_data), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(1)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data), Return(0)));

    page_flipper.schedule_flip(crtc_id, fb_id);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipper.wait_for_flip(crtc_id);
}

TEST_F(KMSPageFlipperTest, wait_for_flip_reports_vsync)
{
    using namespace testing;
    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};
    void* user_data{nullptr};
    ON_CALL(mock_drm, drmModePageFlip(_, _, _, _, _))
        .WillByDefault(DoAll(SaveArg<4>(&user_data), Return(0)));
    ON_CALL(mock_drm, drmHandleEvent(_, _))
        .WillByDefault(DoAll(InvokePageFlipHandler(&user_data), Return(0)));

    EXPECT_CALL(report, report_vsync(crtc_id));

    page_flipper.schedule_flip(crtc_id, fb_id);
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));
    page_flipper.wait_for_flip(crtc_id);
}

TEST_F(KMSPageFlipperTest, wait_for_non_scheduled_page_flip_doesnt_block)
{
    using namespace testing;

    uint32_t const crtc_id{10};

    EXPECT_CALL(mock_drm, drmModePageFlip(_, _, _, _, _))
        .Times(0);

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    page_flipper.wait_for_flip(crtc_id);
}

TEST_F(KMSPageFlipperTest, failure_in_wait_for_flip_throws)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};
    void* user_data{nullptr};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<4>(&user_data), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    page_flipper.schedule_flip(crtc_id, fb_id);

    /* Cause a failure in wait_for_flip */
    close(mock_drm.fake_drm.fd());

    EXPECT_THROW({
        page_flipper.wait_for_flip(crtc_id);
    }, std::runtime_error);
}

TEST_F(KMSPageFlipperTest, wait_for_flips_interleaved)
{
    using namespace testing;

    uint32_t const fb_id{101};
    std::vector<uint32_t> const crtc_ids{10, 11, 12};
    std::vector<void*> user_data{nullptr, nullptr, nullptr};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          _, fb_id, _, _))
        .Times(3)
        .WillOnce(DoAll(SaveArg<4>(&user_data[0]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[1]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[2]), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(3)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[2]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)));

    for (auto crtc_id : crtc_ids)
        page_flipper.schedule_flip(crtc_id, fb_id);

    /* Fake 3 DRM events */
    EXPECT_EQ(3, write(mock_drm.fake_drm.write_fd(), "abc", 3));

    for (auto crtc_id : crtc_ids)
        page_flipper.wait_for_flip(crtc_id);
}

namespace
{

class PageFlippingFunctor
{
public:
    PageFlippingFunctor(mgm::KMSPageFlipper& page_flipper,
                        uint32_t crtc_id)
        : page_flipper(page_flipper), crtc_id{crtc_id}, done{false},
          num_page_flips{0}, num_waits{0}
    {
    }

    void operator()()
    {
        while (!done)
        {
            page_flipper.schedule_flip(crtc_id, 0);
            num_page_flips++;
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            page_flipper.wait_for_flip(crtc_id);
            num_waits++;
        }
    }

    int page_flip_count()
    {
        return num_page_flips;
    }

    int wait_count()
    {
        return num_waits;
    }

    void stop()
    {
        done = true;
    }

private:
    mgm::KMSPageFlipper& page_flipper;
    uint32_t const crtc_id;
    std::atomic<bool> done;
    std::atomic<int> num_page_flips;
    std::atomic<int> num_waits;
};

}

TEST_F(KMSPageFlipperTest, threads_switch_worker)
{
    using namespace testing;

    size_t const worker_index{0};
    size_t const other_index{1};
    std::vector<uint32_t> const crtc_ids{10, 11};
    std::vector<void*> user_data{nullptr, nullptr};
    std::vector<std::unique_ptr<PageFlippingFunctor>> page_flipping_functors;
    std::vector<std::thread> page_flipping_threads;
    std::thread::id tid;

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(), _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SaveArg<4>(&user_data[worker_index]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[other_index]), Return(0)));

    /*
     * The first event releases the original worker, hence we expect that
     * then the other thread will become the worker.
     */
    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(2)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[worker_index]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[other_index]), Return(0)));

    /* Start the page-flipping threads */
    for (auto crtc_id : crtc_ids)
    {
        auto pf = std::unique_ptr<PageFlippingFunctor>(new PageFlippingFunctor{page_flipper, crtc_id});
        page_flipping_functors.push_back(std::move(pf));
        page_flipping_threads.push_back(std::thread{std::ref(*page_flipping_functors.back())});

        /* Wait for page-flip request and tell flipper to stop after this iteration */
        while (page_flipping_functors.back()->page_flip_count() == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        page_flipping_functors.back()->stop();

        /* Wait until the (first) thread has become the worker */
        while (tid == std::thread::id())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            tid = page_flipper.debug_get_worker_tid();
        }
    }

    EXPECT_EQ(page_flipping_threads[worker_index].get_id(), tid);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipping_threads[worker_index].join();

    /* Wait for the worker to switch */
    while (tid != page_flipping_threads[other_index].get_id())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        tid = page_flipper.debug_get_worker_tid();
    }

    /* Fake another DRM event to unblock the remaining thread */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipping_threads[other_index].join();
}

TEST_F(KMSPageFlipperTest, threads_worker_notifies_non_worker)
{
    using namespace testing;

    size_t const worker_index{0};
    size_t const other_index{1};
    std::vector<uint32_t> const crtc_ids{10, 11};
    std::vector<void*> user_data{nullptr, nullptr};
    std::vector<std::unique_ptr<PageFlippingFunctor>> page_flipping_functors;
    std::vector<std::thread> page_flipping_threads;
    std::thread::id tid;

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(), _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SaveArg<4>(&user_data[worker_index]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[other_index]), Return(0)));

    /*
     * The first event releases the non-worker thread, hence we expect that
     * original worker not change.
     */
    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(2)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[other_index]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[worker_index]), Return(0)));

    /* Start the page-flipping threads */
    for (auto crtc_id : crtc_ids)
    {
        auto pf = std::unique_ptr<PageFlippingFunctor>(new PageFlippingFunctor{page_flipper, crtc_id});
        page_flipping_functors.push_back(std::move(pf));
        page_flipping_threads.push_back(std::thread{std::ref(*page_flipping_functors.back())});

        /* Wait for page-flip request and tell flipper to stop after this iteration */
        while (page_flipping_functors.back()->page_flip_count() == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        page_flipping_functors.back()->stop();

        /* Wait until the (first) thread has become the worker */
        while (tid == std::thread::id())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            tid = page_flipper.debug_get_worker_tid();
        }
    }

    EXPECT_EQ(page_flipping_threads[worker_index].get_id(), tid);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    /* Wait for the non-worker thread to exit */
    page_flipping_threads[other_index].join();

    /* Check that the worker hasn't changed */
    EXPECT_EQ(page_flipping_threads[worker_index].get_id(),
              page_flipper.debug_get_worker_tid());

    /* Fake another DRM event to unblock the remaining thread */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipping_threads[worker_index].join();
}

namespace
{

class PageFlipCounter
{
public:
    void add_flip(uint32_t crtc_id, void* user_data)
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        data.push_back({CountType::flip, crtc_id});
        pending_flips.insert(user_data);
    }

    void add_handle_event(uint32_t crtc_id)
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        data.push_back({CountType::handle_event, crtc_id});
    }

    int count_flips()
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        return std::count_if(begin(data), end(data), [](CountElement& e) -> bool
        {
            return e.type == CountType::flip;
        });
    }

    int count_handle_events()
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        return std::count_if(begin(data), end(data), [](CountElement& e) -> bool
        {
            return e.type == CountType::handle_event;
        });
    }

    bool no_consecutive_flips_for_same_crtc_id()
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        std::unordered_set<uint32_t> pending_crtc_ids;

        for (auto& e : data)
        {
            if (e.type == CountType::flip)
            {
                if (pending_crtc_ids.find(e.crtc_id) != pending_crtc_ids.end())
                    return false;

                pending_crtc_ids.insert(e.crtc_id);
            }
            else if (e.type == CountType::handle_event)
            {
                if (pending_crtc_ids.find(e.crtc_id) == pending_crtc_ids.end())
                    return false;
                pending_crtc_ids.erase(e.crtc_id);
            }
        }

        return true;
    }

    void* get_pending_flip_data()
    {
        std::lock_guard<std::mutex> lock{data_mutex};

        auto iter = pending_flips.begin();
        if (iter == pending_flips.end())
        {
            return 0;
        }
        else
        {
            auto d = *iter;
            pending_flips.erase(iter);
            return d;
        }
    }

private:
    enum class CountType {flip, handle_event};
    struct CountElement
    {
        CountType type;
        uint32_t crtc_id;
    };

    std::vector<CountElement> data;
    std::unordered_set<void*> pending_flips;
    std::mutex data_mutex;
};

ACTION_P(InvokePageFlipHandlerWithPendingData, counter)
{
    int const dont_care{0};
    int const drm_fd{arg0};
    char dummy;

    auto user_data = static_cast<mgm::PageFlipEventData*>(counter->get_pending_flip_data());
    uint32_t const crtc_id{user_data->crtc_id};

    /* Remove the event from the drm event queue */
    ASSERT_EQ(1, read(drm_fd, &dummy, 1));
    /* Call the page flip handler */
    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, user_data);
    /* Record this call */
    counter->add_handle_event(crtc_id);
}

ACTION_P2(AddPageFlipEvent, counter, write_drm_fd)
{
    uint32_t const crtc_id{arg0};
    void* const user_data{arg1};

    /* Record this call */
    counter->add_flip(crtc_id, user_data);
    /* Add an event to the drm event queue */
    EXPECT_EQ(1, write(write_drm_fd, "a", 1));
}

}

TEST_F(KMSPageFlipperTest, threads_concurrent_page_flips_dont_deadlock)
{
    using namespace testing;

    std::vector<uint32_t> const crtc_ids{10, 11, 12};
    std::vector<std::unique_ptr<PageFlippingFunctor>> page_flipping_functors;
    std::vector<std::thread> page_flipping_threads;
    PageFlipCounter counter;

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(), _, _, _, _))
        .WillRepeatedly(DoAll(WithArgs<1,4>(AddPageFlipEvent(&counter, mock_drm.fake_drm.write_fd())),
                              Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .WillRepeatedly(DoAll(InvokePageFlipHandlerWithPendingData(&counter),
                              Return(0)));

    /* Set up threads */
    for (auto crtc_id : crtc_ids)
    {
        auto pf = std::unique_ptr<PageFlippingFunctor>(new PageFlippingFunctor{page_flipper, crtc_id});
        page_flipping_functors.push_back(std::move(pf));
        page_flipping_threads.push_back(std::thread{std::ref(*page_flipping_functors.back())});
    }

    /* Wait for at least min_flips page-flips to be processed */
    int const min_flips{100};

    while (counter.count_flips() < min_flips)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});

    /* Tell the flippers to stop and wait for them to finish */
    for (auto& pf : page_flipping_functors)
        pf->stop();

    for (auto& pf_thread : page_flipping_threads)
        pf_thread.join();

    /* Sanity checks */
    EXPECT_EQ(counter.count_flips(), counter.count_handle_events());
    EXPECT_TRUE(counter.no_consecutive_flips_for_same_crtc_id());
}
