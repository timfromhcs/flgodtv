#include "flgod/core/simulation.hpp"
#include "flgod/core/replay.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

int main() {
    std::cout << "[TEST] Running test_headless_modes..." << std::endl;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::filesystem::path cp_path = temp_dir / "flgod_test_cp.json";
    std::filesystem::path replay_path = temp_dir / "flgod_test_replay.json";

    flgod::SimulationConfig cfg;
    cfg.experiment_id = "headless_mode_test";

    // 1. Checkpoint to file & restore verification
    {
        flgod::Simulation sim_orig;
        sim_orig.initialize(cfg);
        sim_orig.run_ticks(150);

        nlohmann::json cp = sim_orig.create_checkpoint();
        std::ofstream out(cp_path);
        if (!out.is_open()) {
            std::cerr << "FAILED: Could not write checkpoint to " << cp_path << std::endl;
            return 1;
        }
        out << cp.dump();
        out.close();

        // Run another 150 ticks on original
        sim_orig.run_ticks(150);
        uint64_t expected_hash = sim_orig.compute_state_hash();

        // Restore in a fresh instance
        std::ifstream in(cp_path);
        nlohmann::json restored_cp;
        in >> restored_cp;
        in.close();

        flgod::Simulation sim_restored;
        sim_restored.initialize(cfg);
        sim_restored.restore_checkpoint(restored_cp);

        if (sim_restored.state().clock().tick() != 150) {
            std::cerr << "FAILED: Restored tick mismatch: " << sim_restored.state().clock().tick() << " != 150" << std::endl;
            return 1;
        }

        sim_restored.run_ticks(150);
        uint64_t restored_final_hash = sim_restored.compute_state_hash();

        if (restored_final_hash != expected_hash) {
            std::cerr << "FAILED: Checkpoint file restore diverged! Expected: 0x" 
                      << std::hex << expected_hash << ", Got: 0x" << restored_final_hash << std::dec << std::endl;
            return 1;
        }
        std::cout << "  Checkpoint file write/restore verified. Hash: 0x" << std::hex << restored_final_hash << std::dec << std::endl;
    }

    // 2. Replay log recording and file verification
    {
        flgod::Simulation sim_rec;
        sim_rec.initialize(cfg);

        flgod::ReplayLog log = flgod::ReplayManager::record(sim_rec, 200, 50);

        // Serialize replay log to file
        std::ofstream out(replay_path);
        out << log.to_json().dump();
        out.close();

        // Read back
        std::ifstream in(replay_path);
        nlohmann::json read_json;
        in >> read_json;
        in.close();

        flgod::ReplayLog read_log;
        read_log.from_json(read_json);

        std::string err;
        if (!flgod::ReplayManager::verify(read_log, &err)) {
            std::cerr << "FAILED: Replay verification from file failed: " << err << std::endl;
            return 1;
        }
        std::cout << "  Replay file recording and verification verified bit-exact for 200 ticks." << std::endl;
    }

    // Cleanup temp files
    std::filesystem::remove(cp_path);
    std::filesystem::remove(replay_path);

    std::cout << "[TEST] test_headless_modes PASSED" << std::endl;
    return 0;
}
