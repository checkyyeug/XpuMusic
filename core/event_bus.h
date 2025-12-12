#pragma once

#include "mp_event.h"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace mp {
namespace core {

// Event subscription entry
struct Subscription {
    SubscriptionHandle handle;
    EventID event_id;
    EventCallback callback;
};

// Event bus implementation
class EventBus : public IEventBus {
public:
    EventBus();
    ~EventBus() override;
    
    // IEventBus implementation
    SubscriptionHandle subscribe(EventID event_id, EventCallback callback) override;
    Result unsubscribe(SubscriptionHandle handle) override;
    Result publish(const Event& event) override;
    Result publish_sync(const Event& event) override;
    
    // Lifecycle
    void start();
    void stop();
    
private:
    void process_events();
    void dispatch_event(const Event& event);
    
    std::unordered_map<SubscriptionHandle, Subscription> subscriptions_;
    std::unordered_map<EventID, std::vector<SubscriptionHandle>> event_map_;
    std::mutex subscription_mutex_;
    
    std::queue<Event> event_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::thread worker_thread_;
    std::atomic<bool> running_;
    
    SubscriptionHandle next_handle_;
};

}} // namespace mp::core
