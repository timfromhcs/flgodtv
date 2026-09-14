#pragma once

// Generic, replaceable rule layer. Rules see a deterministic context view and
// return adjustments; the engine applies them in scenario order. A fly-specific
// rule is scenario data, never an engine assumption.

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

namespace flgod::mpe {

struct RuleEntity {
    uint64_t raw{0};
    std::string archetype;
    double energy{100.0};
    double fatigue{0.0};
    double x{0.0}, y{0.0}, z{0.0};
    std::map<std::string, double> traits; // genome traits (e.g. vigor)
};

struct RuleContext {
    uint64_t tick{0};
    std::vector<RuleEntity> entities; // sorted by raw id (engine guarantees)
    std::vector<std::pair<std::string, uint64_t>> events; // (type, entity)
    double param(const nlohmann::json& cfg, const std::string& k, double dflt) const {
        if (cfg.contains(k) && cfg[k].is_number()) return cfg[k].get<double>();
        return dflt;
    }
};

class IRule {
public:
    virtual ~IRule() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    virtual uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const {
        return nlohmann::json::object();
    }
    virtual void from_json(const nlohmann::json& /*j*/) {}
};

// Foraging: hungry entities find food (+energy, capped). Deterministic.
class ForagingRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "foraging"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double threshold = ctx.param(config, "hunger_below", 60.0);
        double gain = ctx.param(config, "gain", 2.0);
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            if (e.energy < threshold) {
                e.energy += gain;
                if (e.energy > 100.0) e.energy = 100.0;
                ctx.events.emplace_back("FoodConsumed", e.raw);
                ++n;
            }
        }
        return n;
    }
};

// Predation: entities of predator_archetype steal energy from the weakest
// non-predator. Deterministic (sorted order, first-weakest wins ties by id).
class PredationRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "predation"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        std::string predator = "predator";
        if (config.contains("predator_archetype") && config["predator_archetype"].is_string()) {
            predator = config["predator_archetype"].get<std::string>();
        }
        double drain = ctx.param(config, "drain", 5.0);
        double gain = ctx.param(config, "gain", 4.0);
        uint64_t n = 0;
        for (auto& p : ctx.entities) {
            if (p.archetype != predator) continue;
            RuleEntity* weakest = nullptr;
            for (auto& e : ctx.entities) {
                if (e.archetype == predator || e.energy <= 0.0) continue;
                if (!weakest || e.energy < weakest->energy) weakest = &e;
            }
            if (weakest) {
                weakest->energy -= drain;
                if (weakest->energy < 0.0) weakest->energy = 0.0;
                p.energy += gain;
                if (p.energy > 120.0) p.energy = 120.0;
                ctx.events.emplace_back("Predation", p.raw);
                ++n;
            }
        }
        return n;
    }
};

// Cooperation: same-archetype entities average a fraction toward the group mean.
class CooperationRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "cooperation"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double share = ctx.param(config, "share", 0.1);
        std::map<std::string, double> sum;
        std::map<std::string, uint64_t> cnt;
        for (const auto& e : ctx.entities) {
            sum[e.archetype] += e.energy;
            cnt[e.archetype] += 1;
        }
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            double mean = sum[e.archetype] / static_cast<double>(cnt[e.archetype]);
            e.energy += (mean - e.energy) * share;
            ++n;
        }
        return n;
    }
};

// Territory: clamp entities inside max_radius of origin; count violations.
class TerritoryRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "territory"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double max_r = ctx.param(config, "max_radius", 40.0);
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            double r = std::sqrt(e.x * e.x + e.z * e.z);
            if (r > max_r && r > 0.0) {
                e.x *= max_r / r;
                e.z *= max_r / r;
                ctx.events.emplace_back("TerritoryEnforced", e.raw);
                ++n;
            }
        }
        return n;
    }
};

// Reproduction: entities above an energy threshold pay a cost and request
// offspring of their own archetype. The engine performs the actual spawn with
// deterministic IDs (rule only requests; engine executes).
class ReproductionRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "reproduction"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double threshold = ctx.param(config, "energy_above", 95.0);
        double cost = ctx.param(config, "cost", 30.0);
        double cooldown = ctx.param(config, "cooldown_ticks", 20.0);
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            double last = 0.0;
            auto it = m_last_birth.find(e.raw);
            if (it != m_last_birth.end()) last = it->second;
            if (e.energy >= threshold &&
                static_cast<double>(ctx.tick) - last >= cooldown) {
                e.energy -= cost;
                ctx.events.emplace_back("Birth:" + e.archetype, e.raw);
                m_last_birth[e.raw] = static_cast<double>(ctx.tick);
                ++n;
            }
        }
        return n;
    }
    void reset_cooldowns() { m_last_birth.clear(); }
    [[nodiscard]] nlohmann::json to_json() const override {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [k, v] : m_last_birth) j[std::to_string(k)] = v;
        return j;
    }
    void from_json(const nlohmann::json& j) override {
        m_last_birth.clear();
        for (auto& [k, v] : j.items()) {
            m_last_birth[static_cast<uint64_t>(std::stoull(k))] = v.get<double>();
        }
    }

private:
    std::map<uint64_t, double> m_last_birth; // deterministic key order
};

// Survival: entities with critical energy lose extra energy (health pressure).
class SurvivalRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "survival"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double critical = ctx.param(config, "critical_below", 10.0);
        double extra = ctx.param(config, "extra_drain", 0.2);
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            if (e.energy < critical) {
                e.energy -= extra;
                if (e.energy < 0.0) e.energy = 0.0;
                ++n;
            }
        }
        return n;
    }
};

// Deterministic SplitMix64 for rule-local randomness (seeded by tick+entity).
inline uint64_t mpe_splitmix64(uint64_t& s) {
    s += 0x9e3779b97f4a7c15ULL;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// Mutation: perturbs genome traits with per-entity probability. Deterministic:
// stream seeded from (tick, entity id), entities visited in sorted order.
class MutationRule : public IRule {
public:
    [[nodiscard]] std::string name() const override { return "mutation"; }
    uint64_t evaluate(RuleContext& ctx, const nlohmann::json& config) override {
        double prob = ctx.param(config, "probability", 0.05);
        double scale = ctx.param(config, "scale", 0.05);
        std::string trait = "vigor";
        if (config.contains("trait") && config["trait"].is_string()) {
            trait = config["trait"].get<std::string>();
        }
        uint64_t n = 0;
        for (auto& e : ctx.entities) {
            auto it = e.traits.find(trait);
            if (it == e.traits.end()) continue; // no such trait: skip, never invent
            uint64_t s = ctx.tick * 0x9e3779b97f4a7c15ULL + e.raw * 0xbf58476d1ce4e5b9ULL + 1;
            double u = static_cast<double>(mpe_splitmix64(s) >> 11) / 9007199254740992.0;
            if (u < prob) {
                double v = static_cast<double>(mpe_splitmix64(s) >> 11) / 9007199254740992.0;
                double delta = (v * 2.0 - 1.0) * scale;
                it->second += delta;
                if (it->second < 0.0) it->second = 0.0;
                if (it->second > 1.0) it->second = 1.0;
                ctx.events.emplace_back("Mutation", e.raw);
                ++n;
            }
        }
        return n;
    }
};

class RuleRegistry {
public:
    using Factory = std::function<std::unique_ptr<IRule>()>;
    RuleRegistry() {
        register_rule("foraging", []() { return std::make_unique<ForagingRule>(); });
        register_rule("predation", []() { return std::make_unique<PredationRule>(); });
        register_rule("cooperation", []() { return std::make_unique<CooperationRule>(); });
        register_rule("territory", []() { return std::make_unique<TerritoryRule>(); });
        register_rule("survival", []() { return std::make_unique<SurvivalRule>(); });
        register_rule("reproduction", []() { return std::make_unique<ReproductionRule>(); });
        register_rule("mutation", []() { return std::make_unique<MutationRule>(); });
    }
    void register_rule(const std::string& name, Factory f) {
        if (m_factories.count(name)) {
            throw std::runtime_error("RuleRegistry: duplicate rule " + name);
        }
        m_factories[name] = std::move(f);
    }
    [[nodiscard]] std::unique_ptr<IRule> create(const std::string& name) const {
        auto it = m_factories.find(name);
        if (it == m_factories.end()) {
            throw std::runtime_error("RuleRegistry: unknown rule '" + name + "'");
        }
        return it->second();
    }
    [[nodiscard]] bool known(const std::string& name) const {
        return m_factories.count(name) > 0;
    }

private:
    std::map<std::string, Factory> m_factories;
};

} // namespace flgod::mpe
