#pragma once

// MPEEngine: executes a validated Scenario with replaceable brains.
// Deterministic: sorted entity iteration, seeded spawn, fixed system order.

#include "flgod/core/entity_id.hpp"
#include "flgod/mpe/adapters/malecns_brain.hpp"
#include "flgod/mpe/entity.hpp"
#include "flgod/mpe/interfaces.hpp"
#include "flgod/mpe/rules.hpp"
#include "flgod/mpe/scenario.hpp"
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace flgod::mpe {

struct EngineEvent {
    uint64_t tick{0};
    std::string type;
    uint64_t entity_raw{0};
    [[nodiscard]] nlohmann::json to_json() const {
        return {{"tick", tick}, {"type", type}, {"entity_raw", entity_raw}};
    }
};

struct EngineTelemetry {
    uint32_t schema_version{1};
    uint64_t tick{0};
    uint64_t alive{0};
    uint64_t spawned_total{0};
    uint64_t died_total{0};
    uint64_t moves{0};
    uint64_t eats{0};
    uint64_t communicates{0};
    double mean_energy{0.0};
    std::map<std::string, uint64_t> rules_fired;
    [[nodiscard]] nlohmann::json to_json() const {
        return {{"schema_version", schema_version},
                {"tick", tick},
                {"alive", alive},
                {"spawned_total", spawned_total},
                {"died_total", died_total},
                {"moves", moves},
                {"eats", eats},
                {"communicates", communicates},
                {"mean_energy", mean_energy},
                {"rules_fired", rules_fired}};
    }
};

class MPEEngine {
public:
    // Load + validate scenario and archetypes. Throws with clear reason.
    void load(const std::string& scenario_path, const std::string& archetype_dir) {
        m_scenario = Scenario::load(scenario_path);
        m_archetypes.clear();
        for (const auto& pop : m_scenario.populations) {
            if (!m_archetypes.count(pop.archetype)) {
                Archetype a = Archetype::load(archetype_dir + "/" + pop.archetype + ".json");
                if (a.name != pop.archetype) {
                    throw std::runtime_error("Scenario references archetype '" + pop.archetype +
                                             "' but file defines '" + a.name + "'");
                }
                m_archetypes[pop.archetype] = std::move(a);
            }
        }
        m_tick = 0;
        m_rng.seed(m_scenario.master_seed);
        build_rules(); // validate rule names at load; unknown rule throws here
    }

    void build_rules() {
        m_rules.clear();
        m_rule_configs.clear();
        for (const auto& r : m_scenario.rules) {
            m_rules.push_back(m_rule_registry.create(r.name)); // throws if unknown
            m_rule_configs.push_back(r.config);
        }
    }

    void initialize() {
        m_entities = EntityRegistry();
        m_brains.clear();
        m_events.clear();
        m_tick = 0;
        m_spawned = 0;
        m_died = 0;
        m_moves = 0;
        m_eats = 0;
        m_comms = 0;
        m_rule_counts.clear();
        build_rules(); // fresh rule state per run
        uint64_t index = 1;
        for (const auto& pop : m_scenario.populations) {
            const Archetype& a = m_archetypes.at(pop.archetype);
            for (uint64_t i = 0; i < pop.count; ++i) {
                EntityID id(EntityType::Agent, 0, index++);
                Entity& e = m_entities.spawn(id, a.name);
                auto t = std::make_unique<TransformComponent>();
                double ang = uniform(0.0, 6.283185307179586);
                double rad = uniform(0.0, a.spawn_radius);
                t->x = rad * std::cos(ang);
                t->z = rad * std::sin(ang);
                e.attach(std::move(t));
                auto n = std::make_unique<NeedsComponent>();
                if (a.components.contains("needs")) {
                    n->from_json(a.components["needs"]);
                }
                e.attach(std::move(n));
                std::unique_ptr<IBrain> brain = make_brain(a);
                brain->initialize(a.brain_config);
                m_brains[id.raw()] = std::move(brain);
                m_events.push_back(EngineEvent{0, "EntitySpawned", id.raw()});
                m_spawned++;
            }
        }
        m_next_index = index;
    }

    void step() {
        // Perception: build observations (deterministic function of tick+state).
        for (Entity* e : m_entities.ordered()) {
            Observation obs;
            const auto* t = e->get<TransformComponent>("Transform");
            const auto* n = e->get<NeedsComponent>("Needs");
            double temp = 20.0 + 5.0 * std::sin(static_cast<double>(m_tick) * 0.05);
            obs.set("temperature", temp);
            obs.set("energy", n ? n->energy : 100.0);
            obs.set("fatigue", n ? n->fatigue : 0.0);
            obs.set("pos_x", t ? t->x : 0.0);
            obs.set("pos_z", t ? t->z : 0.0);
            obs.set("odor_sugar", temp > 20.0 ? 0.7 : 0.2);
            obs.set("tick", static_cast<double>(m_tick));
            obs.tag("archetype", e->archetype);
            m_brains.at(e->id.raw())->observe(obs);
        }
        // Brain + Decision + ActionValidation + execution.
        std::vector<uint64_t> dead;
        for (Entity* e : m_entities.ordered()) {
            auto& brain = m_brains.at(e->id.raw());
            brain->step(1.0 / 60.0);
            for (ActionIntent in : brain->intents()) {
                in.entity_raw = e->id.raw();
                if (!action_allowed(e->archetype, in.action)) continue; // validated out
                apply(e, in);
            }
            // Biology: metabolic drain.
            auto* n = e->get<NeedsComponent>("Needs");
            if (n) {
                n->energy -= 0.05;
                n->fatigue += 0.01;
                if (n->fatigue > 100.0) n->fatigue = 100.0;
                if (n->energy <= 0.0) {
                    n->energy = 0.0;
                    dead.push_back(e->id.raw());
                }
            }
        }
        for (uint64_t raw : dead) {
            EntityID id(raw);
            m_entities.despawn(id);
            m_brains.erase(raw);
            m_events.push_back(EngineEvent{m_tick, "EntityDied", raw});
            m_died++;
        }
        // Rules: scenario-ordered, deterministic, with write-back + births.
        run_rules();
        m_tick++;
    }

    void run_ticks(uint64_t n) {
        for (uint64_t i = 0; i < n; ++i) step();
    }

    [[nodiscard]] uint64_t tick() const noexcept { return m_tick; }
    [[nodiscard]] const Scenario& scenario() const noexcept { return m_scenario; }
    [[nodiscard]] size_t alive() const { return m_entities.size(); }
    [[nodiscard]] const std::vector<EngineEvent>& events() const noexcept { return m_events; }

    [[nodiscard]] EngineTelemetry telemetry() const {
        EngineTelemetry t;
        t.tick = m_tick;
        t.alive = m_entities.size();
        t.spawned_total = m_spawned;
        t.died_total = m_died;
        t.moves = m_moves;
        t.eats = m_eats;
        t.communicates = m_comms;
        t.rules_fired = m_rule_counts;
        double sum = 0.0;
        // NOTE: ordered() is non-const; telemetry recomputed on a copy-free path.
        for (Entity* e : const_cast<EntityRegistry&>(m_entities).ordered()) {
            const auto* n = e->get<NeedsComponent>("Needs");
            if (n) sum += n->energy;
        }
        t.mean_energy = t.alive ? sum / static_cast<double>(t.alive) : 0.0;
        return t;
    }

    [[nodiscard]] uint64_t compute_hash() const {
        uint64_t h = m_entities.compute_hash();
        h ^= fnv1a64("tick:" + std::to_string(m_tick)) + 0x9e3779b97f4a7c15ULL;
        h ^= fnv1a64("next:" + std::to_string(m_next_index)) + 0x9e3779b97f4a7c15ULL;
        for (const auto& [raw, b] : m_brains) {
            h ^= b->compute_hash() + raw;
            h *= 1099511628211ULL;
        }
        for (const auto& r : m_rules) {
            h ^= fnv1a64(r->name() + ":" + r->to_json().dump());
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json create_checkpoint() const {
        nlohmann::json brains = nlohmann::json::object();
        for (const auto& [raw, b] : m_brains) {
            brains[std::to_string(raw)] = b->to_json();
        }
        nlohmann::json rules = nlohmann::json::array();
        for (const auto& r : m_rules) {
            rules.push_back({{"name", r->name()}, {"state", r->to_json()}});
        }
        return {{"scenario", m_scenario.name},
                {"scenario_version", m_scenario.scenario_version},
                {"master_seed", m_scenario.master_seed},
                {"tick", m_tick},
                {"next_index", m_next_index},
                {"counters",
                 {{"spawned", m_spawned},
                  {"died", m_died},
                  {"moves", m_moves},
                  {"eats", m_eats},
                  {"comms", m_comms},
                  {"rules_fired", m_rule_counts}}},
                {"entities", m_entities.to_json()},
                {"brains", brains},
                {"rules", rules},
                {"state_hash", compute_hash()}};
    }

    void restore_checkpoint(const nlohmann::json& cp) {
        if (!cp.contains("entities") || !cp.contains("brains") || !cp.contains("tick")) {
            throw std::runtime_error("MPEEngine: invalid checkpoint (entities/brains/tick required)");
        }
        if (cp.value("scenario", std::string{}) != m_scenario.name) {
            throw std::runtime_error("MPEEngine: checkpoint scenario mismatch");
        }
        m_entities = EntityRegistry();
        m_brains.clear();
        for (const auto& ej : cp["entities"]) {
            EntityID id(ej.value("id", uint64_t{0}));
            Entity& e = m_entities.spawn(id, ej.value("archetype", std::string{}));
            for (auto& [ctype, cj] : ej["components"].items()) {
                if (ctype == "Transform") {
                    auto c = std::make_unique<TransformComponent>();
                    c->from_json(cj);
                    e.attach(std::move(c));
                } else if (ctype == "Needs") {
                    auto c = std::make_unique<NeedsComponent>();
                    c->from_json(cj);
                    e.attach(std::move(c));
                } else {
                    auto c = std::make_unique<BagComponent>();
                    c->from_json(cj);
                    c->name = ctype;
                    e.attach(std::move(c));
                }
            }
            const Archetype& a = m_archetypes.at(e.archetype);
            std::unique_ptr<IBrain> brain = make_brain(a);
            brain->from_json(cp["brains"].at(std::to_string(id.raw())));
            m_brains[id.raw()] = std::move(brain);
        }
        m_tick = cp["tick"].get<uint64_t>();
        if (cp.contains("counters")) {
            const auto& c = cp["counters"];
            m_spawned = c.value("spawned", m_spawned);
            m_died = c.value("died", m_died);
            m_moves = c.value("moves", m_moves);
            m_eats = c.value("eats", m_eats);
            m_comms = c.value("comms", m_comms);
            m_rule_counts.clear();
            if (c.contains("rules_fired")) {
                for (auto& [k, v] : c["rules_fired"].items()) {
                    m_rule_counts[k] = v.get<uint64_t>();
                }
            }
        }
        m_next_index = cp.value("next_index", uint64_t{1});
        for (Entity* e : m_entities.ordered()) {
            if (e->id.index() >= m_next_index) m_next_index = e->id.index() + 1;
        }
        if (cp.contains("rules")) {
            if (cp["rules"].size() != m_rules.size()) {
                throw std::runtime_error("MPEEngine: checkpoint rule set mismatch");
            }
            for (size_t i = 0; i < m_rules.size(); ++i) {
                if (cp["rules"][i].value("name", std::string{}) != m_rules[i]->name()) {
                    throw std::runtime_error("MPEEngine: checkpoint rule order mismatch");
                }
                m_rules[i]->from_json(cp["rules"][i].value("state", nlohmann::json::object()));
            }
        }
        // Rebuild event log deterministically is out of scope; record restore marker.
        m_events.push_back(EngineEvent{m_tick, "CheckpointRestored", 0});
    }

private:
    std::unique_ptr<IBrain> make_brain(const Archetype& a) {
        if (a.brain_provider == "scripted") {
            return std::make_unique<ScriptedBrain>();
        }
        if (a.brain_provider == "malecns") {
            return std::make_unique<adapters::MaleCNSBrainAdapter>();
        }
        throw std::runtime_error("MPEEngine: unknown brain provider '" + a.brain_provider + "'");
    }

    bool action_allowed(const std::string& archetype, const std::string& action) const {
        const auto& allowed = m_archetypes.at(archetype).actions;
        for (const auto& a : allowed) {
            if (a == action) return true;
        }
        return false;
    }

    void run_rules() {
        if (m_rules.empty()) return;
        RuleContext ctx;
        ctx.tick = m_tick;
        for (Entity* e : m_entities.ordered()) {
            RuleEntity r;
            r.raw = e->id.raw();
            r.archetype = e->archetype;
            const auto* n = e->get<NeedsComponent>("Needs");
            const auto* t = e->get<TransformComponent>("Transform");
            if (n) {
                r.energy = n->energy;
                r.fatigue = n->fatigue;
            }
            if (t) {
                r.x = t->x;
                r.y = t->y;
                r.z = t->z;
            }
            ctx.entities.push_back(r);
        }
        for (size_t i = 0; i < m_rules.size(); ++i) {
            uint64_t fired = m_rules[i]->evaluate(ctx, m_rule_configs[i]);
            m_rule_counts[m_rules[i]->name()] += fired;
        }
        // Write back + births (deterministic order).
        for (const auto& r : ctx.entities) {
            Entity* e = find_entity(r.raw);
            if (!e) continue; // died this tick before rules
            auto* n = e->get<NeedsComponent>("Needs");
            auto* t = e->get<TransformComponent>("Transform");
            if (n) {
                n->energy = r.energy;
                n->fatigue = r.fatigue;
                if (n->energy <= 0.0) {
                    n->energy = 0.0;
                    kill_entity(e->id.raw(), "EntityDied");
                }
            }
            if (t) {
                t->x = r.x;
                t->y = r.y;
                t->z = r.z;
            }
        }
        for (const auto& [type, raw] : ctx.events) {
            if (type.rfind("Birth:", 0) == 0) {
                spawn_offspring(type.substr(6));
            } else {
                m_events.push_back(EngineEvent{m_tick, type, raw});
                if (type == "FoodConsumed") m_eats++;
            }
        }
    }

    Entity* find_entity(uint64_t raw) {
        for (Entity* e : m_entities.ordered()) {
            if (e->id.raw() == raw) return e;
        }
        return nullptr;
    }

    void kill_entity(uint64_t raw, const std::string& cause) {
        EntityID id(raw);
        try {
            m_entities.despawn(id);
        } catch (const std::runtime_error&) {
            return;
        }
        m_brains.erase(raw);
        m_events.push_back(EngineEvent{m_tick, cause, raw});
        m_died++;
    }

    void spawn_offspring(const std::string& archetype) {
        auto it = m_archetypes.find(archetype);
        if (it == m_archetypes.end()) return; // unknown: ignore, never crash
        const Archetype& a = it->second;
        EntityID id(EntityType::Agent, 0, m_next_index++);
        Entity& e = m_entities.spawn(id, a.name);
        auto t = std::make_unique<TransformComponent>();
        e.attach(std::move(t));
        auto n = std::make_unique<NeedsComponent>();
        if (a.components.contains("needs")) n->from_json(a.components["needs"]);
        n->energy *= 0.5; // newborns start weaker
        e.attach(std::move(n));
        std::unique_ptr<IBrain> brain = make_brain(a);
        brain->initialize(a.brain_config);
        m_brains[id.raw()] = std::move(brain);
        m_events.push_back(EngineEvent{m_tick, "Birth", id.raw()});
        m_spawned++;
    }
    void apply(Entity* e, const ActionIntent& in) {
        if (in.action == "Move") {
            auto* t = e->get<TransformComponent>("Transform");
            if (t) {
                auto it = in.params.find("dx");
                if (it != in.params.end()) t->x += it->second;
                it = in.params.find("dy");
                if (it != in.params.end()) t->y += it->second;
                it = in.params.find("dz");
                if (it != in.params.end()) t->z += it->second;
            }
            m_events.push_back(EngineEvent{m_tick, "EntityMoved", e->id.raw()});
            m_moves++;
        } else if (in.action == "Eat") {
            auto* n = e->get<NeedsComponent>("Needs");
            if (n) {
                n->energy += 10.0;
                if (n->energy > 100.0) n->energy = 100.0;
            }
            m_events.push_back(EngineEvent{m_tick, "FoodConsumed", e->id.raw()});
            m_eats++;
        } else if (in.action == "Communicate") {
            m_events.push_back(EngineEvent{m_tick, "Communication", e->id.raw()});
            m_comms++;
        } else if (in.action == "Rest") {
            auto* n = e->get<NeedsComponent>("Needs");
            if (n) {
                n->fatigue -= 1.0;
                if (n->fatigue < 0.0) n->fatigue = 0.0;
            }
        }
    }

    double uniform(double lo, double hi) {
        return lo + (hi - lo) * (static_cast<double>(m_rng()) /
                                 static_cast<double>(m_rng.max()));
    }

    Scenario m_scenario;
    std::map<std::string, Archetype> m_archetypes;
    EntityRegistry m_entities;
    std::map<uint64_t, std::unique_ptr<IBrain>> m_brains; // sorted => deterministic
    RuleRegistry m_rule_registry;
    std::vector<std::unique_ptr<IRule>> m_rules; // scenario order
    std::vector<nlohmann::json> m_rule_configs;
    std::map<std::string, uint64_t> m_rule_counts;
    std::vector<EngineEvent> m_events;
    std::mt19937_64 m_rng;
    uint64_t m_tick{0};
    uint64_t m_next_index{1};
    uint64_t m_spawned{0}, m_died{0}, m_moves{0}, m_eats{0}, m_comms{0};
};

} // namespace flgod::mpe
