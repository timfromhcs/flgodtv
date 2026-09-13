#pragma once

#include "flgod/core/simulation.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace flgod {

struct ReplayCheckpointRecord {
    uint64_t tick{0};
    double elapsed_time{0.0};
    uint64_t state_hash{0};
    nlohmann::json state_json;

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"tick", tick},
            {"elapsed_time", elapsed_time},
            {"state_hash", state_hash},
            {"state", state_json}
        };
    }

    void from_json(const nlohmann::json& j) {
        tick = j.value("tick", 0ULL);
        elapsed_time = j.value("elapsed_time", 0.0);
        state_hash = j.value("state_hash", 0ULL);
        if (j.contains("state")) {
            state_json = j["state"];
        }
    }
};

struct ReplayLog {
    std::string experiment_id{"replay_experiment"};
    SimulationVersion version{CURRENT_SIMULATION_VERSION};
    SimulationConfig config{};
    uint64_t initial_hash{0};
    uint64_t total_ticks{0};
    std::vector<uint64_t> checkpoint_ticks{};
    std::vector<uint64_t> checkpoint_hashes{};
    ReplayCheckpointRecord final_state{};

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["experiment_id"] = experiment_id;
        j["version"] = {
            {"major", version.major},
            {"minor", version.minor},
            {"patch", version.patch},
            {"schema", version.schema_version},
            {"build_meta", version.build_meta}
        };
        j["initial_hash"] = initial_hash;
        j["total_ticks"] = total_ticks;
        j["checkpoint_ticks"] = checkpoint_ticks;
        j["checkpoint_hashes"] = checkpoint_hashes;
        j["final_state"] = final_state.to_json();
        return j;
    }

    void from_json(const nlohmann::json& j) {
        experiment_id = j.value("experiment_id", "replay_experiment");
        if (j.contains("version")) {
            version.major = j["version"].value("major", 0u);
            version.minor = j["version"].value("minor", 1u);
            version.patch = j["version"].value("patch", 0u);
            version.schema_version = j["version"].value("schema", 1u);
            version.build_meta = j["version"].value("build_meta", "dev");
        }
        initial_hash = j.value("initial_hash", 0ULL);
        total_ticks = j.value("total_ticks", 0ULL);
        checkpoint_ticks = j.value("checkpoint_ticks", std::vector<uint64_t>{});
        checkpoint_hashes = j.value("checkpoint_hashes", std::vector<uint64_t>{});
        if (j.contains("final_state")) {
            final_state.from_json(j["final_state"]);
        }
    }
};

class ReplayManager {
public:
    static ReplayLog record(Simulation& sim, uint64_t ticks, uint64_t checkpoint_interval = 60) {
        ReplayLog log;
        log.experiment_id = sim.config().experiment_id;
        log.version = sim.config().version;
        log.config = sim.config();
        log.initial_hash = sim.compute_state_hash();
        log.total_ticks = ticks;

        for (uint64_t t = 1; t <= ticks; ++t) {
            sim.step();
            if (t % checkpoint_interval == 0 || t == ticks) {
                log.checkpoint_ticks.push_back(t);
                log.checkpoint_hashes.push_back(sim.compute_state_hash());
            }
        }

        log.final_state.tick = sim.state().clock().tick();
        log.final_state.elapsed_time = sim.state().clock().elapsed_time();
        log.final_state.state_hash = sim.compute_state_hash();
        log.final_state.state_json = sim.create_checkpoint();
        return log;
    }

    static bool verify(const ReplayLog& log, std::string* error_msg = nullptr) {
        Simulation sim;
        sim.initialize(log.config);

        if (sim.compute_state_hash() != log.initial_hash) {
            if (error_msg) *error_msg = "Initial state hash mismatch before replay!";
            return false;
        }

        size_t cp_idx = 0;
        for (uint64_t t = 1; t <= log.total_ticks; ++t) {
            sim.step();
            if (cp_idx < log.checkpoint_ticks.size() && t == log.checkpoint_ticks[cp_idx]) {
                uint64_t cur_hash = sim.compute_state_hash();
                if (cur_hash != log.checkpoint_hashes[cp_idx]) {
                    if (error_msg) {
                        *error_msg = "Replay divergence at tick " + std::to_string(t) +
                                     ": expected hash 0x" + std::to_string(log.checkpoint_hashes[cp_idx]) +
                                     ", got 0x" + std::to_string(cur_hash);
                    }
                    return false;
                }
                cp_idx++;
            }
        }

        uint64_t final_hash = sim.compute_state_hash();
        if (final_hash != log.final_state.state_hash) {
            if (error_msg) {
                *error_msg = "Final state hash mismatch after replay!";
            }
            return false;
        }

        return true;
    }
};

} // namespace flgod
