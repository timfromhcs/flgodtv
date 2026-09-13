#pragma once

#include "flgod/core/entity_id.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <algorithm>

namespace flgod {

struct BaseEvent {
    virtual ~BaseEvent() = default;
    uint64_t tick{0};
    double timestamp{0.0};
    EntityID sender{NULL_ENTITY};
    [[nodiscard]] virtual std::string type_name() const = 0;
};

template <typename T>
struct Event : public BaseEvent {
    [[nodiscard]] std::string type_name() const override {
        return typeid(T).name();
    }
};

// Core Standard Events
struct TickStartEvent : public Event<TickStartEvent> {};
struct TickEndEvent : public Event<TickEndEvent> {};

struct EntityCreatedEvent : public Event<EntityCreatedEvent> {
    EntityID entity_id{NULL_ENTITY};
    EntityType entity_type{EntityType::Unspecified};
};

struct EntityDestroyedEvent : public Event<EntityDestroyedEvent> {
    EntityID entity_id{NULL_ENTITY};
};

struct CheckpointSavedEvent : public Event<CheckpointSavedEvent> {
    std::string checkpoint_path;
    uint64_t state_hash{0};
};

struct CheckpointLoadedEvent : public Event<CheckpointLoadedEvent> {
    std::string checkpoint_path;
    uint64_t state_hash{0};
};

class EventBus {
public:
    using HandlerFunc = std::function<void(const BaseEvent&)>;

    template <typename T>
    void subscribe(std::function<void(const T&)> handler) {
        std::type_index type_idx = std::type_index(typeid(T));
        m_handlers[type_idx].push_back([handler](const BaseEvent& base_ev) {
            handler(static_cast<const T&>(base_ev));
        });
    }

    template <typename T>
    void publish_immediate(const T& event) {
        std::type_index type_idx = std::type_index(typeid(T));
        auto it = m_handlers.find(type_idx);
        if (it != m_handlers.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }

    template <typename T>
    void enqueue(T event) {
        m_queue.push_back(std::make_unique<T>(std::move(event)));
    }

    size_t flush([[maybe_unused]] uint64_t current_tick = 0) {
        size_t processed = 0;
        std::vector<std::unique_ptr<BaseEvent>> current_batch;
        current_batch.swap(m_queue);

        for (const auto& event_ptr : current_batch) {
            std::type_index type_idx = std::type_index(typeid(*event_ptr));
            auto it = m_handlers.find(type_idx);
            if (it != m_handlers.end()) {
                for (auto& handler : it->second) {
                    handler(*event_ptr);
                }
            }
            processed++;
        }
        return processed;
    }

    void clear_queue() noexcept {
        m_queue.clear();
    }

    void clear_all() noexcept {
        m_queue.clear();
        m_handlers.clear();
    }

    [[nodiscard]] size_t queued_count() const noexcept {
        return m_queue.size();
    }

private:
    std::unordered_map<std::type_index, std::vector<HandlerFunc>> m_handlers;
    std::vector<std::unique_ptr<BaseEvent>> m_queue;
};

} // namespace flgod
