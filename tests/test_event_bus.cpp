#include "../core/event_bus.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>

using namespace mp::core;

class EventBusTest : public ::testing::Test {
protected:
    EventBus* event_bus_;

    void SetUp() override {
        event_bus_ = new EventBus();
        ASSERT_EQ(event_bus_->initialize(), mp::Result::Success);
    }

    void TearDown() override {
        event_bus_->shutdown();
        delete event_bus_;
    }
};

TEST_F(EventBusTest, SubscribeAndPublish) {
    bool callback_invoked = false;
    mp::EventType received_type = 0;
    
    auto handle = event_bus_->subscribe(mp::EVENT_PLAYBACK_STARTED,
        [&](const mp::Event& evt) {
            callback_invoked = true;
            received_type = evt.type;
        }
    );
    
    mp::Event event(mp::EVENT_PLAYBACK_STARTED);
    event_bus_->publish(event);
    
    // Give async event processing time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(received_type, mp::EVENT_PLAYBACK_STARTED);
    
    event_bus_->unsubscribe(handle);
}

TEST_F(EventBusTest, SynchronousPublish) {
    bool callback_invoked = false;
    
    auto handle = event_bus_->subscribe(mp::EVENT_PLAYBACK_STOPPED,
        [&](const mp::Event& evt) {
            callback_invoked = true;
        }
    );
    
    mp::Event event(mp::EVENT_PLAYBACK_STOPPED);
    event_bus_->publish_sync(event);
    
    // Synchronous publish should have already invoked callback
    EXPECT_TRUE(callback_invoked);
    
    event_bus_->unsubscribe(handle);
}

TEST_F(EventBusTest, MultipleSubscribers) {
    int callback_count = 0;
    
    auto handle1 = event_bus_->subscribe(mp::EVENT_PLAYBACK_PAUSED,
        [&](const mp::Event&) { callback_count++; }
    );
    
    auto handle2 = event_bus_->subscribe(mp::EVENT_PLAYBACK_PAUSED,
        [&](const mp::Event&) { callback_count++; }
    );
    
    auto handle3 = event_bus_->subscribe(mp::EVENT_PLAYBACK_PAUSED,
        [&](const mp::Event&) { callback_count++; }
    );
    
    mp::Event event(mp::EVENT_PLAYBACK_PAUSED);
    event_bus_->publish_sync(event);
    
    EXPECT_EQ(callback_count, 3);
    
    event_bus_->unsubscribe(handle1);
    event_bus_->unsubscribe(handle2);
    event_bus_->unsubscribe(handle3);
}

TEST_F(EventBusTest, Unsubscribe) {
    bool callback_invoked = false;
    
    auto handle = event_bus_->subscribe(mp::EVENT_TRACK_CHANGED,
        [&](const mp::Event&) { callback_invoked = true; }
    );
    
    event_bus_->unsubscribe(handle);
    
    mp::Event event(mp::EVENT_TRACK_CHANGED);
    event_bus_->publish_sync(event);
    
    EXPECT_FALSE(callback_invoked);
}

TEST_F(EventBusTest, MultipleEventTypes) {
    int playback_started_count = 0;
    int playback_stopped_count = 0;
    
    auto handle1 = event_bus_->subscribe(mp::EVENT_PLAYBACK_STARTED,
        [&](const mp::Event&) { playback_started_count++; }
    );
    
    auto handle2 = event_bus_->subscribe(mp::EVENT_PLAYBACK_STOPPED,
        [&](const mp::Event&) { playback_stopped_count++; }
    );
    
    mp::Event start_event(mp::EVENT_PLAYBACK_STARTED);
    mp::Event stop_event(mp::EVENT_PLAYBACK_STOPPED);
    
    event_bus_->publish_sync(start_event);
    event_bus_->publish_sync(stop_event);
    event_bus_->publish_sync(start_event);
    
    EXPECT_EQ(playback_started_count, 2);
    EXPECT_EQ(playback_stopped_count, 1);
    
    event_bus_->unsubscribe(handle1);
    event_bus_->unsubscribe(handle2);
}

TEST_F(EventBusTest, AsyncPublishThreadSafety) {
    std::atomic<int> callback_count{0};
    
    auto handle = event_bus_->subscribe(mp::EVENT_PLAYBACK_STARTED,
        [&](const mp::Event&) { callback_count++; }
    );
    
    // Publish from multiple threads
    const int num_threads = 10;
    const int events_per_thread = 100;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < events_per_thread; ++j) {
                mp::Event event(mp::EVENT_PLAYBACK_STARTED);
                event_bus_->publish(event);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for all async events to process
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_EQ(callback_count, num_threads * events_per_thread);
    
    event_bus_->unsubscribe(handle);
}

TEST_F(EventBusTest, EventDataPassing) {
    std::string received_data;
    
    auto handle = event_bus_->subscribe(mp::EVENT_PLAYBACK_STARTED,
        [&](const mp::Event& evt) {
            if (evt.data && evt.data_size > 0) {
                received_data = std::string(static_cast<const char*>(evt.data), evt.data_size);
            }
        }
    );
    
    const std::string test_data = "test_payload";
    mp::Event event(mp::EVENT_PLAYBACK_STARTED);
    event.data = test_data.c_str();
    event.data_size = test_data.length();
    
    event_bus_->publish_sync(event);
    
    EXPECT_EQ(received_data, test_data);
    
    event_bus_->unsubscribe(handle);
}
