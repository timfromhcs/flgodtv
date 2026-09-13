#include <cassert>
#include <iostream>
#include <filesystem>
#include "flgod/brain/malecns_adapter.hpp"

int main() {
    std::cout << "[Test] Running MaleCNS Connectome Dataset Loader Unit Tests...\n";

    std::string csv_path = "malecns/data-raw/2023-27-2 soma_sides.csv";
    assert(std::filesystem::exists(csv_path) && "malecns connectome dataset file must exist!");

    // Load up to 5,000 reconstructed biological neurons from the real Janelia connectome dataset
    const size_t TARGET_SOMAS = 5000;
    flgod::brain::MaleCNSAdapter adapter(csv_path, TARGET_SOMAS);

    assert(adapter.soma_count() >= TARGET_SOMAS && "Should have loaded 5,000 connectome somas");
    std::cout << "  Loaded " << adapter.soma_count() << " biological somas from Janelia connectome.\n";

    // 1. Verify coordinate realistic bounds (Fly brain is ~500-600 um wide)
    const auto& somas = adapter.somas();
    size_t left_count = 0;
    size_t right_count = 0;

    for (const auto& s : somas) {
        assert(s.body_id != 0);
        assert(s.nucleus_id != 0);
        assert(s.x_um > 0.0 && s.x_um < 1000.0 && "X coordinate must be within realistic CNS bounds");
        assert(s.y_um > 0.0 && s.y_um < 1000.0 && "Y coordinate must be within realistic CNS bounds");
        assert(s.z_um > 0.0 && s.z_um < 1000.0 && "Z coordinate must be within realistic CNS bounds");
        if (s.is_left_hemisphere) left_count++;
        else right_count++;
    }

    // 2. Verify bilateral hemisphere representation
    assert(left_count > 1000 && "Left hemisphere must have substantial soma representation");
    assert(right_count > 1000 && "Right hemisphere must have substantial soma representation");
    std::cout << "  Hemisphere balance: Left=" << left_count << ", Right=" << right_count << "\n";

    // 3. Step execution with real connectome somas
    flgod::brain::BrainSensoryInput input;
    input.odor_sugar_intensity = 0.5;
    flgod::brain::BrainMotorOutput out;
    adapter.step(0.01, input, out);

    assert(out.left_wing_freq_hz > 150.0);
    assert(out.right_wing_freq_hz > 150.0);
    assert(out.proboscis_extension > 0.0);

    std::cout << "  Connectome execution step verified: Wingbeat=" << out.left_wing_freq_hz 
              << " Hz, PER=" << out.proboscis_extension << "\n";
    std::cout << "[Test] MaleCNS Connectome Dataset Loader Unit Tests PASSED!\n";
    return 0;
}
