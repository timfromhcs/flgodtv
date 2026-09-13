#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include "flgod/core/rng.hpp"

namespace flgod::llm {

constexpr uint32_t GGUF_MAGIC = 0x46554747; // "GGUF" in little-endian

enum class GGUFValueType : uint32_t {
    UINT8 = 0,
    INT8 = 1,
    UINT16 = 2,
    INT16 = 3,
    UINT32 = 4,
    INT32 = 5,
    FLOAT32 = 6,
    BOOL = 7,
    STRING = 8,
    ARRAY = 9,
    UINT64 = 10,
    INT64 = 11,
    FLOAT64 = 12
};

struct GGUFMetadataValue {
    GGUFValueType type{GGUFValueType::STRING};
    std::variant<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, 
                 float, bool, std::string, uint64_t, int64_t, double,
                 std::vector<std::string>> val;

    std::string as_string() const {
        if (std::holds_alternative<std::string>(val)) {
            return std::get<std::string>(val);
        } else if (std::holds_alternative<uint32_t>(val)) {
            return std::to_string(std::get<uint32_t>(val));
        } else if (std::holds_alternative<uint64_t>(val)) {
            return std::to_string(std::get<uint64_t>(val));
        } else if (std::holds_alternative<int32_t>(val)) {
            return std::to_string(std::get<int32_t>(val));
        } else if (std::holds_alternative<float>(val)) {
            return std::to_string(std::get<float>(val));
        } else if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
        return "";
    }

    uint64_t as_uint64() const {
        if (std::holds_alternative<uint64_t>(val)) return std::get<uint64_t>(val);
        if (std::holds_alternative<uint32_t>(val)) return std::get<uint32_t>(val);
        if (std::holds_alternative<int32_t>(val)) return static_cast<uint64_t>(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val)) return static_cast<uint64_t>(std::get<int64_t>(val));
        return 0;
    }
};

struct GGUFHeader {
    uint32_t magic{0};
    uint32_t version{0};
    uint64_t tensor_count{0};
    uint64_t metadata_kv_count{0};
};

struct GGUFModelInfo {
    GGUFHeader header;
    std::unordered_map<std::string, GGUFMetadataValue> metadata;
    std::string architecture;
    std::string name;
    std::string quantization_type{"Q4_K_M"};
    uint64_t context_length{2048};
    uint64_t embedding_length{4096};
    uint64_t block_count{32};
    uint64_t file_size_bytes{0};
    uint64_t content_hash{0};
    std::string model_hash_hex;

    bool has_key(const std::string& key) const {
        return metadata.find(key) != metadata.end();
    }

    std::string get_string(const std::string& key, const std::string& default_val = "") const {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            return it->second.as_string();
        }
        return default_val;
    }

    uint64_t get_uint64(const std::string& key, uint64_t default_val = 0) const {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            return it->second.as_uint64();
        }
        return default_val;
    }
};

class GGUFParser {
public:
    static bool parse_file(const std::string& filepath, GGUFModelInfo& out_info) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        out_info.file_size_bytes = static_cast<uint64_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        if (out_info.file_size_bytes < sizeof(uint32_t) * 2 + sizeof(uint64_t) * 2) {
            return false;
        }

        // Read header
        file.read(reinterpret_cast<char*>(&out_info.header.magic), sizeof(uint32_t));
        if (out_info.header.magic != GGUF_MAGIC) {
            return false;
        }

        file.read(reinterpret_cast<char*>(&out_info.header.version), sizeof(uint32_t));
        if (out_info.header.version < 1 || out_info.header.version > 3) {
            return false;
        }

        file.read(reinterpret_cast<char*>(&out_info.header.tensor_count), sizeof(uint64_t));
        file.read(reinterpret_cast<char*>(&out_info.header.metadata_kv_count), sizeof(uint64_t));

        // Read metadata key-values
        uint64_t rolling_hash = 14695981039346656037ULL; // FNV offset basis
        for (uint64_t i = 0; i < out_info.header.metadata_kv_count; ++i) {
            if (file.eof() || file.fail()) break;

            // Read key string
            uint64_t key_len = 0;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(uint64_t));
            if (key_len > 4096 || file.fail()) break;

            std::string key(key_len, '\0');
            file.read(&key[0], key_len);

            // Read value type
            uint32_t vtype = 0;
            file.read(reinterpret_cast<char*>(&vtype), sizeof(uint32_t));

            GGUFMetadataValue mval;
            mval.type = static_cast<GGUFValueType>(vtype);

            switch (mval.type) {
                case GGUFValueType::UINT8: {
                    uint8_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::INT8: {
                    int8_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::UINT16: {
                    uint16_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::INT16: {
                    int16_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::UINT32: {
                    uint32_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::INT32: {
                    int32_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::FLOAT32: {
                    float v = 0.0f; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::BOOL: {
                    uint8_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = (v != 0); break;
                }
                case GGUFValueType::STRING: {
                    uint64_t slen = 0;
                    file.read(reinterpret_cast<char*>(&slen), sizeof(slen));
                    if (slen < 65536) {
                        std::string s(slen, '\0');
                        file.read(&s[0], slen);
                        mval.val = s;
                    }
                    break;
                }
                case GGUFValueType::UINT64: {
                    uint64_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::INT64: {
                    int64_t v = 0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::FLOAT64: {
                    double v = 0.0; file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    mval.val = v; break;
                }
                case GGUFValueType::ARRAY: {
                    uint32_t elem_type = 0;
                    uint64_t elem_count = 0;
                    file.read(reinterpret_cast<char*>(&elem_type), sizeof(elem_type));
                    file.read(reinterpret_cast<char*>(&elem_count), sizeof(elem_count));
                    if (static_cast<GGUFValueType>(elem_type) == GGUFValueType::STRING && elem_count < 1024) {
                        std::vector<std::string> arr;
                        for (uint64_t a = 0; a < elem_count; ++a) {
                            uint64_t str_l = 0;
                            file.read(reinterpret_cast<char*>(&str_l), sizeof(str_l));
                            std::string elem_s(str_l, '\0');
                            file.read(&elem_s[0], str_l);
                            arr.push_back(elem_s);
                        }
                        mval.val = arr;
                    }
                    break;
                }
            }

            // Update rolling hash with key and value string
            for (char c : key) {
                rolling_hash ^= static_cast<uint64_t>(c);
                rolling_hash *= 1099511628211ULL;
            }
            std::string val_str = mval.as_string();
            for (char c : val_str) {
                rolling_hash ^= static_cast<uint64_t>(c);
                rolling_hash *= 1099511628211ULL;
            }

            out_info.metadata[key] = mval;
        }

        // Populate summary fields from standard GGUF keys
        out_info.architecture = out_info.get_string("general.architecture", "llama");
        out_info.name = out_info.get_string("general.name", "unnamed_model");
        out_info.quantization_type = out_info.get_string("general.file_type", "Q4_K_M");
        
        std::string ctx_key = out_info.architecture + ".context_length";
        out_info.context_length = out_info.get_uint64(ctx_key, out_info.get_uint64("general.context_length", 2048));

        std::string emb_key = out_info.architecture + ".embedding_length";
        out_info.embedding_length = out_info.get_uint64(emb_key, out_info.get_uint64("general.embedding_length", 4096));

        std::string blk_key = out_info.architecture + ".block_count";
        out_info.block_count = out_info.get_uint64(blk_key, 32);

        out_info.content_hash = rolling_hash;
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << rolling_hash;
        out_info.model_hash_hex = ss.str();

        return true;
    }

    // Helper to generate a minimal valid GGUF binary file for testing
    static bool create_synthetic_gguf(const std::string& filepath,
                                     const std::string& name,
                                     const std::string& arch,
                                     uint64_t context_len = 2048,
                                     uint64_t tensor_count = 10) {
        std::ofstream out(filepath, std::ios::binary);
        if (!out.is_open()) return false;

        uint32_t magic = GGUF_MAGIC;
        uint32_t version = 3;
        uint64_t tensors = tensor_count;
        uint64_t kv_count = 4;

        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&tensors), sizeof(tensors));
        out.write(reinterpret_cast<const char*>(&kv_count), sizeof(kv_count));

        auto write_kv_str = [&](const std::string& key, const std::string& val) {
            uint64_t klen = key.size();
            out.write(reinterpret_cast<const char*>(&klen), sizeof(klen));
            out.write(key.data(), klen);
            uint32_t vt = static_cast<uint32_t>(GGUFValueType::STRING);
            out.write(reinterpret_cast<const char*>(&vt), sizeof(vt));
            uint64_t vlen = val.size();
            out.write(reinterpret_cast<const char*>(&vlen), sizeof(vlen));
            out.write(val.data(), vlen);
        };

        auto write_kv_u64 = [&](const std::string& key, uint64_t val) {
            uint64_t klen = key.size();
            out.write(reinterpret_cast<const char*>(&klen), sizeof(klen));
            out.write(key.data(), klen);
            uint32_t vt = static_cast<uint32_t>(GGUFValueType::UINT64);
            out.write(reinterpret_cast<const char*>(&vt), sizeof(vt));
            out.write(reinterpret_cast<const char*>(&val), sizeof(val));
        };

        write_kv_str("general.architecture", arch);
        write_kv_str("general.name", name);
        write_kv_str("general.file_type", "Q4_K_M");
        write_kv_u64(arch + ".context_length", context_len);

        out.close();
        return true;
    }
};

} // namespace flgod::llm
