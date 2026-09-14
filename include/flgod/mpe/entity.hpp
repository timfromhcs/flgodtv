#pragma once

// Generic entity + component registries. Deterministic, serializable, hashable.

#include "flgod/core/entity_id.hpp"
#include "flgod/mpe/mpe_types.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace flgod::mpe {

// A component is versioned JSON-backed data. Typed helpers may layer on top;
// the registry only needs name/version/json/hash.
struct Component {
    virtual ~Component() = default;
    [[nodiscard]] virtual std::string type_name() const = 0;
    [[nodiscard]] virtual uint32_t version() const { return MPE_COMPONENT_SCHEMA_VERSION; }
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
    [[nodiscard]] virtual uint64_t compute_hash() const {
        return fnv1a64(type_name() + ":" + to_json().dump());
    }
};

// Generic bag component: arbitrary named doubles/strings for Custom data.
struct BagComponent : public Component {
    std::string name{"Custom"};
    std::map<std::string, double> numbers;
    std::map<std::string, std::string> strings;
    [[nodiscard]] std::string type_name() const override { return name; }
    [[nodiscard]] nlohmann::json to_json() const override {
        return {{"name", name}, {"numbers", numbers}, {"strings", strings}};
    }
    void from_json(const nlohmann::json& j) override {
        name = j.value("name", std::string{"Custom"});
        numbers.clear();
        strings.clear();
        if (j.contains("numbers")) {
            for (auto& [k, v] : j["numbers"].items()) numbers[k] = v.get<double>();
        }
        if (j.contains("strings")) {
            for (auto& [k, v] : j["strings"].items()) strings[k] = v.get<std::string>();
        }
    }
};

struct TransformComponent : public Component {
    double x{0.0}, y{0.0}, z{0.0};
    double qx{0.0}, qy{0.0}, qz{0.0}, qw{1.0};
    [[nodiscard]] std::string type_name() const override { return "Transform"; }
    [[nodiscard]] nlohmann::json to_json() const override {
        return {{"pos", {x, y, z}}, {"quat", {qx, qy, qz, qw}}};
    }
    void from_json(const nlohmann::json& j) override {
        const auto& p = j.at("pos");
        x = p[0].get<double>();
        y = p[1].get<double>();
        z = p[2].get<double>();
        const auto& q = j.at("quat");
        qx = q[0].get<double>();
        qy = q[1].get<double>();
        qz = q[2].get<double>();
        qw = q[3].get<double>();
    }
};

struct NeedsComponent : public Component {
    double energy{100.0}, hydration{100.0}, health{100.0}, fatigue{0.0};
    [[nodiscard]] std::string type_name() const override { return "Needs"; }
    [[nodiscard]] nlohmann::json to_json() const override {
        return {{"energy", energy}, {"hydration", hydration},
                {"health", health}, {"fatigue", fatigue}};
    }
    void from_json(const nlohmann::json& j) override {
        energy = j.value("energy", 100.0);
        hydration = j.value("hydration", 100.0);
        health = j.value("health", 100.0);
        fatigue = j.value("fatigue", 0.0);
    }
};

class ComponentRegistry {
public:
    using Factory = std::function<std::unique_ptr<Component>()>;
    void register_type(const std::string& name, Factory f) {
        if (m_factories.count(name)) {
            throw std::runtime_error("ComponentRegistry: duplicate type " + name);
        }
        m_factories[name] = std::move(f);
    }
    [[nodiscard]] std::unique_ptr<Component> create(const std::string& name) const {
        auto it = m_factories.find(name);
        if (it == m_factories.end()) {
            throw std::runtime_error("ComponentRegistry: unknown type " + name);
        }
        return it->second();
    }
    [[nodiscard]] bool known(const std::string& name) const {
        return m_factories.count(name) > 0;
    }

private:
    std::map<std::string, Factory> m_factories; // sorted => deterministic
};

struct Entity {
    EntityID id;
    std::string archetype;
    std::map<std::string, std::unique_ptr<Component>> components; // sorted

    [[nodiscard]] bool has(const std::string& type) const {
        return components.count(type) > 0;
    }
    template <typename T>
    [[nodiscard]] T* get(const std::string& type) {
        auto it = components.find(type);
        return it != components.end() ? static_cast<T*>(it->second.get()) : nullptr;
    }
    template <typename T>
    [[nodiscard]] const T* get(const std::string& type) const {
        auto it = components.find(type);
        return it != components.end() ? static_cast<const T*>(it->second.get()) : nullptr;
    }
    void attach(std::unique_ptr<Component> c) {
        components[c->type_name()] = std::move(c);
    }
    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json comps = nlohmann::json::object();
        for (const auto& [k, v] : components) comps[k] = v->to_json();
        return {{"id", id.raw()}, {"archetype", archetype}, {"components", comps}};
    }
};

class EntityRegistry {
public:
    Entity& spawn(EntityID id, const std::string& archetype) {
        if (m_entities.count(id.raw())) {
            throw std::runtime_error("EntityRegistry: duplicate spawn");
        }
        auto e = std::make_unique<Entity>();
        e->id = id;
        e->archetype = archetype;
        Entity& ref = *e;
        m_entities[id.raw()] = std::move(e);
        return ref;
    }
    void despawn(EntityID id) {
        auto it = m_entities.find(id.raw());
        if (it == m_entities.end()) {
            throw std::runtime_error("EntityRegistry: despawn of unknown entity");
        }
        m_entities.erase(it);
    }
    [[nodiscard]] size_t size() const noexcept { return m_entities.size(); }
    // Deterministic iteration: sorted by raw EntityID.
    [[nodiscard]] std::vector<Entity*> ordered() {
        std::vector<Entity*> out;
        out.reserve(m_entities.size());
        for (auto& [k, v] : m_entities) out.push_back(v.get());
        return out;
    }
    [[nodiscard]] uint64_t compute_hash() const {
        uint64_t h = 1469598103934665603ULL;
        for (const auto& [k, v] : m_entities) {
            h ^= fnv1a64(v->to_json().dump());
            h *= 1099511628211ULL;
        }
        return h;
    }
    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [k, v] : m_entities) arr.push_back(v->to_json());
        return arr;
    }

private:
    std::map<uint64_t, std::unique_ptr<Entity>> m_entities; // sorted
};

} // namespace flgod::mpe
