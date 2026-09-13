#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "flgod/llm/model_manager.hpp"
#include "flgod/llm/action_security.hpp"
#include "flgod/llm/god_fly.hpp"
#include "flgod/llm/npc_system.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << "  FLGODTV LLM & ACTION SECURITY BENCHMARK\n";
    std::cout << "========================================================\n";

    // 1. ModelManager Startup & Allocation Benchmark
    auto t_start = std::chrono::steady_clock::now();
    flgod::llm::ModelManager mgr(8192.0, 4096.0);
    
    mgr.load_model(flgod::llm::ModelRole::GodFly, "godfly.gguf", flgod::llm::LLMBackend::CPU, true);
    mgr.load_model(flgod::llm::ModelRole::NPC, "npc.gguf", flgod::llm::LLMBackend::Vulkan, true);
    
    auto t_loaded = std::chrono::steady_clock::now();
    double load_time_ms = std::chrono::duration<double, std::milli>(t_loaded - t_start).count();

    std::cout << "Model Load Time: " << load_time_ms << " ms\n";
    std::cout << "Allocated RAM:   " << mgr.get_budget().current_ram_mb << " MB\n";
    std::cout << "Allocated VRAM:  " << mgr.get_budget().current_vram_mb << " MB\n";

    // 2. Inference & Token Generation Throughput Benchmark
    const int INFERENCE_ITERATIONS = 1000;
    auto t_infer_start = std::chrono::steady_clock::now();
    
    uint64_t total_tokens = 0;
    for (int i = 0; i < INFERENCE_ITERATIONS; ++i) {
        std::string resp = mgr.generate_response(
            flgod::llm::ModelRole::GodFly, 
            "Instruction prompt for fly agent",
            "",
            48
        );
        total_tokens += (resp.size() / 4 + 1);
    }

    auto t_infer_end = std::chrono::steady_clock::now();
    double total_infer_ms = std::chrono::duration<double, std::milli>(t_infer_end - t_infer_start).count();
    double tokens_per_sec = (static_cast<double>(total_tokens) / (total_infer_ms / 1000.0));
    double avg_latency_ms = total_infer_ms / INFERENCE_ITERATIONS;

    std::cout << "Total Inferences: " << INFERENCE_ITERATIONS << "\n";
    std::cout << "Total Tokens:     " << total_tokens << "\n";
    std::cout << "Throughput:       " << tokens_per_sec << " tokens/sec\n";
    std::cout << "Avg Latency:      " << avg_latency_ms << " ms/call\n";

    // 3. Action Security Validation Throughput Benchmark (5-stage pipeline)
    flgod::llm::ActionSecurityValidator validator;
    const int VALIDATION_ITERATIONS = 10000;
    std::string test_payload = "{\n"
        "  \"action\": \"concept\",\n"
        "  \"parameters\": {\n"
        "    \"concept_id\": \"foraging_technique_flowers\",\n"
        "    \"content\": \"Prioritize high sugar flower nectar.\",\n"
        "    \"target_agent\": 1\n"
        "  }\n"
        "}";

    auto t_val_start = std::chrono::steady_clock::now();
    int passed_count = 0;
    for (int i = 0; i < VALIDATION_ITERATIONS; ++i) {
        auto res = validator.validate_and_parse(test_payload, flgod::EntityID(100), true);
        if (res.passed) passed_count++;
    }
    auto t_val_end = std::chrono::steady_clock::now();
    double val_total_ms = std::chrono::duration<double, std::milli>(t_val_end - t_val_start).count();
    double validations_per_sec = (static_cast<double>(VALIDATION_ITERATIONS) / (val_total_ms / 1000.0));

    std::cout << "Security Pipeline Validations: " << passed_count << " / " << VALIDATION_ITERATIONS << "\n";
    std::cout << "Validation Speed:              " << validations_per_sec << " checks/sec\n";

    // 4. Output JSON Evidence
    nlohmann::json bench_json;
    bench_json["load_time_ms"] = load_time_ms;
    bench_json["allocated_ram_mb"] = mgr.get_budget().current_ram_mb;
    bench_json["allocated_vram_mb"] = mgr.get_budget().current_vram_mb;
    bench_json["inference_iterations"] = INFERENCE_ITERATIONS;
    bench_json["total_tokens_generated"] = total_tokens;
    bench_json["tokens_per_second"] = tokens_per_sec;
    bench_json["average_latency_ms"] = avg_latency_ms;
    bench_json["validation_iterations"] = VALIDATION_ITERATIONS;
    bench_json["validations_per_second"] = validations_per_sec;

    std::filesystem::create_directories("evidence/windows");
    std::string out_path = "evidence/windows/llm_benchmark.json";
    std::ofstream out(out_path);
    out << bench_json.dump(2) << std::endl;
    out.close();

    std::cout << "Saved benchmark evidence to: " << out_path << "\n";
    std::cout << "========================================================\n";
    return 0;
}
