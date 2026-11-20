#pragma once

#include "aether/core/status.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace aether::core {

struct Subscription final {
    std::uint64_t value{0};
    friend bool operator==(Subscription, Subscription) = default;
};

class EventDispatcher final {
public:
    template<class Event>
    using Handler = std::function<void(const Event&)>;

    template<class Event>
    Subscription subscribe(Handler<Event> handler) {
        if (!handler) return {};
        const Subscription subscription{next_subscription_++};
        auto erased = [callback = std::move(handler)](const void* event) {
            callback(*static_cast<const Event*>(event));
        };
        auto callback = std::make_shared<std::function<void(const void*)>>(std::move(erased));
        handlers_[std::type_index(typeid(Event))].push_back(
            Entry{subscription, std::move(callback)});
        return subscription;
    }

    template<class Event>
    [[nodiscard]] Status dispatch(const Event& event) {
        const auto type = std::type_index(typeid(Event));
        const auto found = handlers_.find(type);
        if (found == handlers_.end()) return Status::success();

        std::vector<Subscription> snapshot;
        snapshot.reserve(found->second.size());
        for (const auto& entry : found->second) snapshot.push_back(entry.subscription);

        std::size_t failures = 0;
        for (const auto subscription : snapshot) {
            const auto current = handlers_.find(type);
            if (current == handlers_.end()) break;
            const auto entry = std::find_if(current->second.begin(), current->second.end(),
                [subscription](const Entry& candidate) {
                    return candidate.subscription == subscription;
                });
            if (entry == current->second.end()) continue;

            // Retain the subscribed callable while invoking it. This preserves mutable
            // handler state and keeps self-unsubscription safe.
            const auto callback = entry->callback;
            try {
                (*callback)(&event);
            } catch (...) {
                ++failures;
            }
        }

        if (failures != 0) {
            return Status::failure(ErrorCode::InternalError,
                std::to_string(failures) + " event handler(s) failed.");
        }
        return Status::success();
    }

    void unsubscribe(Subscription subscription) noexcept {
        if (subscription.value == 0) return;
        for (auto& [type, entries] : handlers_) {
            (void)type;
            std::erase_if(entries, [subscription](const Entry& entry) {
                return entry.subscription == subscription;
            });
        }
    }

private:
    struct Entry final {
        Subscription subscription;
        std::shared_ptr<std::function<void(const void*)>> callback;
    };

    std::unordered_map<std::type_index, std::vector<Entry>> handlers_;
    std::uint64_t next_subscription_{1};
};

} // namespace aether::core
