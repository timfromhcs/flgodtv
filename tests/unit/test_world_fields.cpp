#include "flgod/world/fields.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_world_fields..." << std::endl;

    // 1. Bilinear sampling verification
    // 4x4 grid, cell_size = 2.0, origin = (0, 0)
    flgod::ScalarField2D field(4, 4, 2.0, 0.0, 0.0, 0.0);
    field.set_grid(0, 0, 0.0);
    field.set_grid(1, 0, 10.0);
    field.set_grid(0, 1, 10.0);
    field.set_grid(1, 1, 20.0);

    // Exact center between (0,0) and (1,1) at world coordinate (1.0, 1.0)
    double sampled_center = field.sample(1.0, 1.0);
    double expected_center = 10.0;
    if (std::abs(sampled_center - expected_center) > 1e-6) {
        std::cerr << "FAILED: Bilinear interpolation error: expected " << expected_center
                  << ", got " << sampled_center << std::endl;
        return 1;
    }

    // 2. WindField vector sampling
    flgod::WindField wind(4, 4, 2.0);
    wind.set(0, 0, flgod::Vec3(5.0, 0.0, 2.0));
    flgod::Vec3 w_samp = wind.sample(0.0, 0.0);
    if (std::abs(w_samp.x - 5.0) > 1e-6 || std::abs(w_samp.z - 2.0) > 1e-6) {
        std::cerr << "FAILED: Wind field vector sample mismatch" << std::endl;
        return 1;
    }

    // 3. FireField simulation test: burning fuel & extinguishing
    flgod::FireField fire(8, 8, 1.0);
    flgod::WindField calm_wind(8, 8, 1.0);
    flgod::MoistureField dry_ground(8, 8, 1.0, 0.0, 0.0, 0.1);

    // Ignite cell (3, 3)
    fire.intensity().set_grid(3, 3, 0.9);
    fire.fuel().set_grid(3, 3, 0.5);

    double init_fuel = fire.fuel().get_grid(3, 3);
    for (int step = 0; step < 10; ++step) {
        fire.step_fire(0.5, calm_wind, dry_ground);
    }
    double burned_fuel = fire.fuel().get_grid(3, 3);
    if (burned_fuel >= init_fuel) {
        std::cerr << "FAILED: Fire failed to consume fuel: before=" << init_fuel << ", after=" << burned_fuel << std::endl;
        return 1;
    }

    // 4. EcologyField test: biomass growth under warm, moist conditions
    flgod::EcologyField ecology(8, 8, 1.0);
    flgod::TemperatureField warm_temp(8, 8, 1.0, 0.0, 0.0, 24.0); // 24 C optimal
    flgod::MoistureField good_moist(8, 8, 1.0, 0.0, 0.0, 0.7);    // 0.7 good moisture

    ecology.biomass().set_grid(2, 2, 0.2);
    for (int step = 0; step < 20; ++step) {
        ecology.step_ecology(1.0, warm_temp, good_moist);
    }
    double new_biomass = ecology.biomass().get_grid(2, 2);
    if (new_biomass <= 0.2) {
        std::cerr << "FAILED: Biomass failed to grow under optimal ecological conditions!" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_world_fields passed successfully." << std::endl;
    return 0;
}
