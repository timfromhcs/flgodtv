#include <cassert>
#include <iostream>
#include <filesystem>
#include "flgod/llm/gguf_parser.hpp"

int main() {
    std::cout << "[Test] Running GGUF Parser Unit Tests...\n";

    std::string test_dir = "test_data_gguf";
    std::filesystem::create_directories(test_dir);
    std::string test_file = test_dir + "/test_model.gguf";

    // 1. Create synthetic valid GGUF file
    bool created = flgod::llm::GGUFParser::create_synthetic_gguf(
        test_file,
        "FLGOD-Test-Llama-1B",
        "llama",
        4096,
        140
    );
    assert(created && "Failed to create synthetic GGUF file");
    (void)created;

    // 2. Parse the GGUF file
    flgod::llm::GGUFModelInfo info;
    bool parsed = flgod::llm::GGUFParser::parse_file(test_file, info);
    assert(parsed && "Failed to parse synthetic GGUF file");
    (void)parsed;

    // 3. Verify header fields
    assert(info.header.magic == flgod::llm::GGUF_MAGIC && "Magic mismatch");
    assert(info.header.version == 3 && "Version mismatch");
    assert(info.header.tensor_count == 140 && "Tensor count mismatch");
    assert(info.header.metadata_kv_count == 4 && "KV count mismatch");

    // 4. Verify extracted metadata
    assert(info.architecture == "llama" && "Architecture mismatch");
    assert(info.name == "FLGOD-Test-Llama-1B" && "Name mismatch");
    assert(info.quantization_type == "Q4_K_M" && "Quantization mismatch");
    assert(info.context_length == 4096 && "Context length mismatch");
    assert(info.content_hash != 0 && "Content hash should be non-zero");
    assert(!info.model_hash_hex.empty() && "Model hash hex should not be empty");

    std::cout << "  Parsed model: " << info.name << " (" << info.architecture << ")\n";
    std::cout << "  Context length: " << info.context_length << ", Hash: " << info.model_hash_hex << "\n";

    // 5. Corrupt file tests
    std::string corrupt_file = test_dir + "/corrupt.gguf";
    {
        std::ofstream out(corrupt_file, std::ios::binary);
        uint32_t bad_magic = 0x12345678;
        out.write(reinterpret_cast<const char*>(&bad_magic), sizeof(bad_magic));
    }
    flgod::llm::GGUFModelInfo corrupt_info;
    bool bad_parsed = flgod::llm::GGUFParser::parse_file(corrupt_file, corrupt_info);
    assert(!bad_parsed && "Corrupt magic should be rejected");
    (void)bad_parsed;

    // Cleanup
    std::filesystem::remove_all(test_dir);

    std::cout << "[Test] GGUF Parser Unit Tests PASSED!\n";
    return 0;
}
