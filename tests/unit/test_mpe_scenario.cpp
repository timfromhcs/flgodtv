#include "flgod/mpe/scenario.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

#define CHECK(cond, msg) do { if (!(cond)) { \
    std::cerr << "FAILED: " << msg << std::endl; return 1; } } while (0)

using namespace flgod::mpe;

static std::string repo_prefix() {
    for (const std::string& c : {"", "../", "../../", "../../../"}) {
        if (std::filesystem::exists(c + "scenarios/01_drosophila_ecosystem.json")) {
            return c;
        }
    }
    return "";
}

int main() {
    std::cout << "[TEST] Running test_mpe_scenario..." << std::endl;
    std::string pre = repo_prefix();
    CHECK(!pre.empty(), "must locate scenarios/ from test CWD");

    // Valid scenario + archetype
    Scenario s = Scenario::load(pre + "scenarios/01_drosophila_ecosystem.json");
    CHECK(s.name == "drosophila_ecosystem", "scenario name");
    CHECK(s.master_seed == 1337, "master seed");
    CHECK(s.populations.size() == 1 && s.populations[0].count == 6, "population");
    CHECK(s.rules.size() == 2, "rules loaded");
    Archetype a = Archetype::load(pre + "scenarios/archetypes/fly.json");
    CHECK(a.name == "fly", "archetype name");
    CHECK(a.brain_provider == "malecns", "fly brain provider");
    CHECK(a.actions.size() == 4, "fly actions");

    // Missing file
    bool missing = false;
    try { Scenario::load("scenarios/does_not_exist.json"); }
    catch (const std::runtime_error&) { missing = true; }
    CHECK(missing, "missing scenario file must throw");

    // Malformed JSON
    const std::string bad_path = "build_mpe_bad.json";
    { std::ofstream f(bad_path); f << "{not json"; }
    bool malformed = false;
    try { Scenario::load(bad_path); }
    catch (const std::runtime_error&) { malformed = true; }
    CHECK(malformed, "malformed JSON must throw");
    std::filesystem::remove(bad_path);

    // Missing required keys
    const std::string nokeys = "build_mpe_nokeys.json";
    { std::ofstream f(nokeys); f << "{\"name\":\"x\"}"; }
    bool nokey = false;
    try { Scenario::load(nokeys); }
    catch (const std::runtime_error&) { nokey = true; }
    CHECK(nokey, "missing keys must throw");
    std::filesystem::remove(nokeys);

    // Empty populations
    const std::string nopop = "build_mpe_nopop.json";
    { std::ofstream f(nopop); f << "{\"name\":\"x\",\"scenario_version\":1,\"master_seed\":1,\"populations\":[]}"; }
    bool nopop_err = false;
    try { Scenario::load(nopop); }
    catch (const std::runtime_error&) { nopop_err = true; }
    CHECK(nopop_err, "empty populations must throw");
    std::filesystem::remove(nopop);

    // Archetype with empty actions
    const std::string noact = "build_mpe_noact.json";
    { std::ofstream f(noact); f << "{\"name\":\"blob\",\"actions\":[]}"; }
    bool noact_err = false;
    try { Archetype::load(noact); }
    catch (const std::runtime_error&) { noact_err = true; }
    CHECK(noact_err, "empty archetype actions must throw");
    std::filesystem::remove(noact);

    std::cout << "[PASS] test_mpe_scenario passed successfully." << std::endl;
    return 0;
}
