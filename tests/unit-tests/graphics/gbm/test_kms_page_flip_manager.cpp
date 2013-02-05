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

#include "src/graphics/gbm/kms_page_flip_manager.h"

#include "mock_drm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <thread>
#include <unordered_set>

namespace mgg = mir::graphics::gbm;

namespace
{

class KMSPageFlipManagerTest : public ::testing::Test
{
public:
    KMSPageFlipManagerTest()
        : pf_manager{mock_drm.fake_drm.fd(), std::chrono::milliseconds{50}},
          blocking_pf_manager{mock_drm.fake_drm.fd(), std::chrono::hours{1}}
    {
    }

    testing::NiceMock<mgg::MockDRM> mock_drm;
    mgg::KMSPageFlipManager pf_manager;
    mgg::KMSPageFlipManager blocking_pf_manager;
};

ACTION_P(InvokePageFlipHandler, param)
{
    int const dont_care{0};
    char dummy;

    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, *param);
    ASSERT_EQ(1, read(arg0, &dummy, 1));
}

}

TEST_F(KMSPageFlipManagerTest, schedule_page_flip_calls_drm_page_flip)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    pf_manager.schedule_page_flip(crtc_id, fb_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flip_handles_drm_event)
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

    pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flip_doesnt_block_forever_if_no_drm_event_comes)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* No DRM event, should time out */

    pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_non_scheduled_page_flip_doesnt_block)
{
    using namespace testing;

    uint32_t const crtc_id{10};

    EXPECT_CALL(mock_drm, drmModePageFlip(_, _, _, _, _))
        .Times(0);

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    blocking_pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flips_interleaved)
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
        pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* Fake 3 DRM events */
    EXPECT_EQ(3, write(mock_drm.fake_drm.write_fd(), "abc", 3));

    for (auto crtc_id : crtc_ids)
        pf_manager.wait_for_page_flip(crtc_id);
}

namespace
{

class PageFlipper
{
public:
    PageFlipper(mgg::KMSPageFlipManager& pf_manager,
                uint32_t crtc_id)
        : pf_manager(pf_manager), crtc_id{crtc_id}, done{false},
          num_page_flips{0}, num_waits{0}
    {
    }

    void operator()()
    {
        while (!done)
        {
            pf_manager.schedule_page_flip(crtc_id, 0);
            num_page_flips++;
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            pf_manager.wait_for_page_flip(crtc_id);
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
    mgg::KMSPageFlipManager& pf_manager;
    uint32_t const crtc_id;
    std::atomic<bool> done;
    std::atomic<int> num_page_flips;
    std::atomic<int> num_waits;
};

}

TEST_F(KMSPageFlipManagerTest, threads_switch_loop_master)
{
    using namespace testing;

    std::vector<uint32_t> const crtc_ids{10, 11};
    std::vector<void*> user_data{nullptr, nullptr};
    std::vector<std::unique_ptr<PageFlipper>> page_flippers;
    std::vector<std::thread> page_flipper_threads;
    std::thread::id tid;

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(), _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SaveArg<4>(&user_data[0]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[1]), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(2)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)));

    /* Start the page-flipping threads */
    for (auto crtc_id : crtc_ids)
    {
        auto pf = std::unique_ptr<PageFlipper>(new PageFlipper{blocking_pf_manager, crtc_id});
        page_flippers.push_back(std::move(pf));
        page_flipper_threads.push_back(std::thread{std::ref(*page_flippers.back())});

        /* Wait for page-flip request and tell flipper to stop after this iteration */
        while (page_flippers.back()->page_flip_count() == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        page_flippers.back()->stop();
    }

    /* Wait until one of the threads has become the loop master */
    while (tid == std::thread::id())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        tid = blocking_pf_manager.debug_get_wait_loop_master();
    }

    EXPECT_TRUE(tid == page_flipper_threads[0].get_id() ||
                tid == page_flipper_threads[1].get_id());

    size_t loop_master_index{0};
    size_t other_index{1};
    if (tid == page_flipper_threads[1].get_id())
        std::swap(loop_master_index, other_index);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipper_threads[loop_master_index].join();

    /* Wait for the master to switch */
    while (tid != page_flipper_threads[other_index].get_id())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        tid = blocking_pf_manager.debug_get_wait_loop_master();
    }

    /* Fake another DRM event to unblock the remaining thread */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipper_threads[other_index].join();
}

TEST_F(KMSPageFlipManagerTest, threads_loop_master_notifies_non_master)
{
    using namespace testing;

    std::vector<uint32_t> const crtc_ids{10, 11};
    std::vector<void*> user_data{nullptr, nullptr};
    std::vector<std::unique_ptr<PageFlipper>> page_flippers;
    std::vector<std::thread> page_flipper_threads;
    std::thread::id tid;

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(), _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SaveArg<4>(&user_data[0]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[1]), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(2)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)));

    /* Start the page-flipping threads */
    for (auto crtc_id : crtc_ids)
    {
        auto pf = std::unique_ptr<PageFlipper>(new PageFlipper{blocking_pf_manager, crtc_id});
        page_flippers.push_back(std::move(pf));
        page_flipper_threads.push_back(std::thread{std::ref(*page_flippers.back())});

        /* Wait for page-flip request and tell flipper to stop after this iteration */
        while (page_flippers.back()->page_flip_count() == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        page_flippers.back()->stop();
    }

    /* Wait until one of the threads has become the loop master */
    while (tid == std::thread::id())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        tid = blocking_pf_manager.debug_get_wait_loop_master();
    }

    EXPECT_TRUE(tid == page_flipper_threads[0].get_id() ||
                tid == page_flipper_threads[1].get_id());

    size_t loop_master_index{0};
    size_t other_index{1};
    if (tid == page_flipper_threads[1].get_id())
        std::swap(loop_master_index, other_index);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    /* Wait for the non-master thread to exit */
    page_flipper_threads[other_index].join();

    /* Check that the the master hasn't changed */
    EXPECT_EQ(page_flipper_threads[loop_master_index].get_id(),
              blocking_pf_manager.debug_get_wait_loop_master());

    /* Fake another DRM event to unblock the remaining thread */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    page_flipper_threads[loop_master_index].join();
}

namespace
{

class PageFlipCounter
{
public:
    void add_flip(uint32_t crtc_id, void* user_data)
    {
        data.push_back({CountType::flip, crtc_id});
        pending_flips.insert(user_data);
    }

    void add_handle_event(int crtc_id)
    {
        data.push_back({CountType::handle_event, crtc_id});
    }

    int count_flips()
    {
        return std::count_if(begin(data), end(data), [](CountElement& e) -> bool
        {
            return e.type == CountType::flip;
        });
    }

    int count_handle_events()
    {
        return std::count_if(begin(data), end(data), [](CountElement& e) -> bool
        {
            return e.type == CountType::handle_event;
        });
    }

    bool no_consecutive_flips_for_same_crtc_id()
    {
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
};

ACTION_P(InvokePageFlipHandlerWithPendingData, counter)
{
    int const dont_care{0};
    int const drm_fd{arg0};
    char dummy;

    auto user_data = static_cast<mgg::PageFlipEventData*>(counter->get_pending_flip_data());
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
    int const crtc_id{arg0};
    void* const user_data{arg1};

    /* Record this call */
    counter->add_flip(crtc_id, user_data);
    /* Add an event to the drm event queue */
    EXPECT_EQ(1, write(write_drm_fd, "a", 1));
}

}

TEST_F(KMSPageFlipManagerTest, threads_concurrent_page_flips_dont_deadlock)
{
    using namespace testing;

    std::vector<uint32_t> const crtc_ids{10, 11, 12};
    std::vector<std::unique_ptr<PageFlipper>> page_flippers;
    std::vector<std::thread> page_flipper_threads;
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
        auto pf = std::unique_ptr<PageFlipper>(new PageFlipper{blocking_pf_manager, crtc_id});
        page_flippers.push_back(std::move(pf));
        page_flipper_threads.push_back(std::thread{std::ref(*page_flippers.back())});
    }

    /* Wait for at least min_flips page-flips to be processed */
    int const min_flips{100};

    while (counter.count_flips() < min_flips)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});

    /* Tell the flippers to stop and wait for them to finish */
    for (auto& pf : page_flippers)
        pf->stop();

    for (auto& pf_thread : page_flipper_threads)
        pf_thread.join();

    /* Sanity checks */
    EXPECT_EQ(counter.count_flips(), counter.count_handle_events());
    EXPECT_TRUE(counter.no_consecutive_flips_for_same_crtc_id());
}
