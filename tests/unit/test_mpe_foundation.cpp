#include "flgod/mpe/mpe_types.hpp"
#include "flgod/mpe/observation.hpp"
#include "flgod/mpe/entity.hpp"
#include "flgod/mpe/interfaces.hpp"
#include <iostream>
#include <string>
#include <vector>

#define CHECK(cond, msg) do { if (!(cond)) { \
    std::cerr << "FAILED: " << msg << std::endl; return 1; } } while (0)

using namespace flgod;
using namespace flgod::mpe;

// Test sensor reading a temperature channel from an env view (no mutation).
class TempSensor : public ISensor {
public:
    [[nodiscard]] std::string name() const override { return "TemperatureSensor"; }
    [[nodiscard]] Observation sample(uint64_t entity_raw,
                                    const nlohmann::json& env_view) const override {
        Observation o;
        o.set("temperature", env_view.value("temperature", 0.0));
        o.set("entity", static_cast<double>(entity_raw % 1000));
        return o;
    }
};

// Minimal action validator: allowlist + per-action param check.
static ValidationResult validate_intent(const ActionIntent& in) {
    static const std::vector<std::string> allowed{"Move", "Eat", "Rest", "Communicate"};
    for (const auto& a : allowed) {
        if (in.action == a) {
            if (in.action == "Move" && in.params.find("dx") == in.params.end()) {
                return {false, "Move requires dx"};
            }
            return {true, "ok"};
        }
    }
    return {false, "unknown action " + in.action};
}

int main() {
    std::cout << "[TEST] Running test_mpe_foundation..." << std::endl;

    // --- 1. Entity registry: spawn, deterministic order, despawn, errors ---
    EntityRegistry reg;
    EntityID a(EntityType::Agent, 0, 30), b(EntityType::Agent, 0, 7), c(EntityType::Agent, 0, 15);
    reg.spawn(a, "fly");
    reg.spawn(b, "ant");
    reg.spawn(c, "robot");
    CHECK(reg.size() == 3, "registry size 3");
    auto ordered = reg.ordered();
    CHECK(ordered.size() == 3, "ordered size 3");
    CHECK(ordered[0]->id.raw() == b.raw(), "sorted first = smallest id");
    CHECK(ordered[1]->id.raw() == c.raw(), "sorted second");
    CHECK(ordered[2]->id.raw() == a.raw(), "sorted third");
    bool dup = false;
    try { reg.spawn(a, "fly"); } catch (const std::runtime_error&) { dup = true; }
    CHECK(dup, "duplicate spawn must throw");
    uint64_t h1 = reg.compute_hash();
    reg.despawn(b);
    CHECK(reg.size() == 2, "despawn reduces size");
    CHECK(reg.compute_hash() != h1, "hash changes after despawn");
    bool missing = false;
    try { reg.despawn(b); } catch (const std::runtime_error&) { missing = true; }
    CHECK(missing, "despawn of unknown must throw");

    // --- 2. Components: registry, attach, roundtrip, unknown type ---
    ComponentRegistry creg;
    creg.register_type("Transform", []() -> std::unique_ptr<Component> {
        return std::make_unique<TransformComponent>();
    });
    creg.register_type("Needs", []() -> std::unique_ptr<Component> {
        return std::make_unique<NeedsComponent>();
    });
    bool duptype = false;
    try { creg.register_type("Transform", []() -> std::unique_ptr<Component> {
        return std::make_unique<TransformComponent>();
    }); } catch (const std::runtime_error&) { duptype = true; }
    CHECK(duptype, "duplicate component type must throw");
    bool unknown = false;
    try { auto unused = creg.create("Nope"); (void)unused; } catch (const std::runtime_error&) { unknown = true; }
    CHECK(unknown, "unknown component type must throw");

    Entity& e = reg.spawn(EntityID(EntityType::Agent, 0, 99), "fly");
    auto t = creg.create("Transform");
    static_cast<TransformComponent*>(t.get())->x = 1.5;
    e.attach(std::move(t));
    auto n = creg.create("Needs");
    e.attach(std::move(n));
    CHECK(e.has("Transform") && e.has("Needs"), "components attached");
    CHECK(e.get<TransformComponent>("Transform")->x == 1.5, "typed get works");
    CHECK(e.get<TransformComponent>("Missing") == nullptr, "missing get is null");
    // JSON roundtrip of one component
    nlohmann::json tj = e.get<TransformComponent>("Transform")->to_json();
    TransformComponent t2;
    t2.from_json(tj);
    CHECK(t2.x == 1.5 && t2.qw == 1.0, "transform roundtrip");
    // Registry hash determinism: same composition => same hash
    CHECK(reg.compute_hash() == reg.compute_hash(), "registry hash stable");

    // --- 3. Observation / ActionIntent roundtrip ---
    Observation obs;
    obs.set("temperature", 21.5);
    obs.tag("biome", "meadow");
    Observation obs2;
    obs2.from_json(obs.to_json());
    CHECK(obs2.get("temperature") == 21.5, "observation channel roundtrip");
    CHECK(obs2.get("missing", -1.0) == -1.0, "observation fallback");
    ActionIntent in;
    in.action = "Move";
    in.entity_raw = 42;
    in.params["dx"] = 1.0;
    ActionIntent in2;
    in2.from_json(in.to_json());
    CHECK(in2.action == "Move" && in2.entity_raw == 42 && in2.params["dx"] == 1.0,
          "intent roundtrip");
    CHECK(validate_intent(in2).accepted, "Move with dx accepted");
    in2.params.clear();
    CHECK(!validate_intent(in2).accepted, "Move without dx rejected");
    in2.action = "DeleteWorld";
    CHECK(!validate_intent(in2).accepted, "unknown action rejected");

    // --- 4. Sensor: deterministic + non-mutating ---
    TempSensor sensor;
    nlohmann::json env_view = {{"temperature", 18.0}};
    Observation s1 = sensor.sample(7, env_view);
    Observation s2 = sensor.sample(7, env_view);
    CHECK(s1.to_json().dump() == s2.to_json().dump(), "sensor deterministic");
    CHECK(env_view == nlohmann::json({{"temperature", 18.0}}), "sensor must not mutate env");
    CHECK(s1.get("temperature") == 18.0, "sensor reads env channel");
    Observation s3 = sensor.sample(8, env_view);
    CHECK(s3.to_json().dump() != s1.to_json().dump(), "sensor distinguishes entities");

    // --- 5. ScriptedBrain: intents, serialize, hash, reset ---
    ScriptedBrain brain;
    brain.initialize({{"action", "Eat"}});
    brain.observe(s1);
    brain.step(1.0 / 60.0);
    auto intents = brain.intents();
    CHECK(intents.size() == 1 && intents[0].action == "Eat", "scripted intent");
    CHECK(validate_intent(intents[0]).accepted, "scripted intent validates");
    uint64_t bh1 = brain.compute_hash();
    brain.step(1.0 / 60.0);
    CHECK(brain.compute_hash() != bh1, "brain hash advances with steps");
    nlohmann::json bj = brain.to_json();
    ScriptedBrain brain_b;
    brain_b.from_json(bj);
    CHECK(brain_b.compute_hash() == brain.compute_hash(), "brain serialize roundtrip");
    brain_b.reset();
    ScriptedBrain fresh;
    fresh.initialize({{"action", "Eat"}});
    CHECK(brain_b.compute_hash() == fresh.compute_hash(), "reset restores initial");

    // --- 6. Scheduler: explicit order, disable, errors ---
    SimulationScheduler sched;
    std::vector<std::string> seen;
    sched.register_system(Stage::Physics, "phys",
        [&](Stage, uint64_t, double) { seen.push_back("Physics"); });
    sched.register_system(Stage::Environment, "env",
        [&](Stage, uint64_t, double) { seen.push_back("Environment"); });
    sched.register_system(Stage::Brain, "brain",
        [&](Stage, uint64_t, double) { seen.push_back("Brain"); });
    sched.step(0, 1.0 / 60.0);
    CHECK(seen.size() == 3, "all systems ran");
    CHECK(seen[0] == "Environment" && seen[1] == "Brain" && seen[2] == "Physics",
          "stage order enforced");
    sched.set_enabled("brain", false);
    seen.clear();
    sched.step(1, 1.0 / 60.0);
    CHECK(seen.size() == 2 && seen[0] == "Environment" && seen[1] == "Physics",
          "disabled system skipped");
    auto order = sched.order();
    CHECK(order.size() == 3, "order() inspectable");
    bool dupe = false;
    try { sched.register_system(Stage::Social, "phys", [&](Stage, uint64_t, double) {}); }
    catch (const std::runtime_error&) { dupe = true; }
    CHECK(dupe, "duplicate system name must throw");
    bool nosys = false;
    try { sched.set_enabled("nope", true); } catch (const std::runtime_error&) { nosys = true; }
    CHECK(nosys, "unknown system toggle must throw");

    std::cout << "[PASS] test_mpe_foundation passed successfully." << std::endl;
    return 0;
}
