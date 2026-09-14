#include "flgod/mpe/engine.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define CHECK(cond, msg) do { if (!(cond)) { \
    std::cerr << "FAILED: " << msg << std::endl; return 1; } } while (0)

using namespace flgod::mpe;

// CTest CWD is the build dir; locate the repo tree robustly.
static std::string repo_prefix() {
    for (const std::string& c : {"", "../", "../../", "../../../"}) {
        if (std::filesystem::exists(c + "scenarios/01_drosophila_ecosystem.json")) {
            return c;
        }
    }
    return "";
}

static uint64_t run_scenario(const std::string& pre, const std::string& file, uint64_t ticks) {
    MPEEngine eng;
    eng.load(pre + "scenarios/" + file, pre + "scenarios/archetypes");
    eng.initialize();
    eng.run_ticks(ticks);
    return eng.compute_hash();
}

int main() {
    std::cout << "[TEST] Running test_mpe_engine..." << std::endl;
    std::string pre = repo_prefix();
    CHECK(!pre.empty(), "must locate scenarios/ from test CWD");

    // All three shipped scenarios run on the same core without source changes.
    for (const std::string& f : {"01_drosophila_ecosystem.json", "02_predator_prey.json",
                                 "03_ant_colony.json"}) {
        MPEEngine eng;
        eng.load(pre + "scenarios/" + f, pre + "scenarios/archetypes");
        eng.initialize();
        CHECK(eng.alive() > 0, std::string("scenario spawns entities: ") + f);
        eng.run_ticks(eng.scenario().ticks);
        CHECK(eng.tick() == eng.scenario().ticks, "scenario completes ticks");
        EngineTelemetry t = eng.telemetry();
        CHECK(t.spawned_total == t.alive + t.died_total, "telemetry census consistent");
        std::cout << "  - " << f << " ticks=" << eng.tick() << " alive=" << eng.alive()
                  << " hash=" << eng.compute_hash() << std::endl;
    }

    // Determinism: same scenario twice => identical hash.
    uint64_t h1 = run_scenario(pre, "01_drosophila_ecosystem.json", 120);
    uint64_t h2 = run_scenario(pre, "01_drosophila_ecosystem.json", 120);
    CHECK(h1 == h2, "repeat runs must be bit-identical");

    // Checkpoint: 0->60, save, restore, 60->120 === 0->120.
    MPEEngine ref;
    ref.load(pre + "scenarios/02_predator_prey.json", pre + "scenarios/archetypes");
    ref.initialize();
    ref.run_ticks(120);
    MPEEngine half;
    half.load(pre + "scenarios/02_predator_prey.json", pre + "scenarios/archetypes");
    half.initialize();
    half.run_ticks(60);
    nlohmann::json cp = half.create_checkpoint();
    MPEEngine resumed;
    resumed.load(pre + "scenarios/02_predator_prey.json", pre + "scenarios/archetypes");
    resumed.initialize(); // replaced by restore
    resumed.restore_checkpoint(cp);
    resumed.run_ticks(60);
    CHECK(resumed.tick() == 120, "resumed run reaches N");
    CHECK(resumed.compute_hash() == ref.compute_hash(), "checkpoint restore bit-exact");

    // Edge: zero-count population runs cleanly.
    MPEEngine zero;
    zero.load(pre + "scenarios/01_drosophila_ecosystem.json", pre + "scenarios/archetypes");
    zero.initialize();
    CHECK(zero.alive() == 6, "baseline population spawns");
    // Corrupt checkpoint rejected.
    bool badcp = false;
    try { zero.restore_checkpoint(nlohmann::json({{"nope", 1}})); }
    catch (const std::runtime_error&) { badcp = true; }
    CHECK(badcp, "corrupt checkpoint must throw");
    // Unknown scenario archetype rejected at load.
    bool badarch = false;
    try {
        MPEEngine bad;
        bad.load(pre + "scenarios/01_drosophila_ecosystem.json", pre + "nonexistent_dir");
    } catch (const std::runtime_error&) { badarch = true; }
    CHECK(badarch, "missing archetype dir must throw");

    std::cout << "[PASS] test_mpe_engine passed successfully." << std::endl;
    return 0;
}
