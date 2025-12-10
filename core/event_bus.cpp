#include "event_bus.h"
#include <chrono>
#include <algorithm>

namespace mp {
namespace core {

EventBus::EventBus()
    : running_(false)
    , next_handle_(1) {
}

EventBus::~EventBus() {
    stop();
}

void EventBus::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    worker_thread_ = std::thread(&EventBus::process_events, this);
}

void EventBus::stop() {
    if (!running_.exchange(false)) {
        return; // Not running
    }
    
    queue_cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

SubscriptionHandle EventBus::subscribe(EventID event_id, EventCallback callback) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    
    SubscriptionHandle handle = next_handle_++;
    
    Subscription sub;
    sub.handle = handle;
    sub.event_id = event_id;
    sub.callback = std::move(callback);
    
    subscriptions_[handle] = std::move(sub);
    event_map_[event_id].push_back(handle);
    
    return handle;
}

Result EventBus::unsubscribe(SubscriptionHandle handle) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    
    auto it = subscriptions_.find(handle);
    if (it == subscriptions_.end()) {
        return Result::InvalidParameter;
    }
    
    EventID event_id = it->second.event_id;
    subscriptions_.erase(it);
    
    // Remove from event map
    auto& handles = event_map_[event_id];
    handles.erase(std::remove(handles.begin(), handles.end(), handle), handles.end());
    
    if (handles.empty()) {
        event_map_.erase(event_id);
    }
    
    return Result::Success;
}

Result EventBus::publish(const Event& event) {
    Event evt = event;
    evt.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        event_queue_.push(evt);
    }
    
    queue_cv_.notify_one();
    return Result::Success;
}

Result EventBus::publish_sync(const Event& event) {
    Event evt = event;
    evt.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    dispatch_event(evt);
    return Result::Success;
}

void EventBus::process_events() {
    while (running_) {
        Event event;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !event_queue_.empty() || !running_; });
            
            if (!running_) {
                break;
            }
            
            if (event_queue_.empty()) {
                continue;
            }
            
            event = event_queue_.front();
            event_queue_.pop();
        }
        
        dispatch_event(event);
    }
}

void EventBus::dispatch_event(const Event& event) {
    std::vector<EventCallback> callbacks;
    
    {
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        
        auto it = event_map_.find(event.id);
        if (it != event_map_.end()) {
            for (SubscriptionHandle handle : it->second) {
                auto sub_it = subscriptions_.find(handle);
                if (sub_it != subscriptions_.end()) {
                    callbacks.push_back(sub_it->second.callback);
                }
            }
        }
    }
    
    // Call callbacks outside of lock
    for (const auto& callback : callbacks) {
        try {
            callback(event);
        } catch (...) {
            // Ignore exceptions in event handlers
        }
    }
}

}} // namespace mp::core
