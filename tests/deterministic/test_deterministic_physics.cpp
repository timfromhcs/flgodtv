#include "flgod/physics/physics_engine.hpp"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "[TEST] Running test_deterministic_physics..." << std::endl;

    auto setup_engine = []() {
        flgod::PhysicsEngine engine(flgod::Vec3(0.0, -9.81, 0.0));

        // Create a set of diverse bodies
        for (uint32_t i = 1; i <= 10; ++i) {
            flgod::RigidBodyState body;
            body.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, i);
            body.position = flgod::Vec3(i * 1.5 - 7.5, 10.0 + i * 0.5, 0.0);
            body.linear_velocity = flgod::Vec3((i % 2 == 0 ? 1.0 : -1.0) * (i * 0.2), 0.0, 0.0);
            body.mass = 0.5 + i * 0.25;
            body.restitution = 0.4 + (i % 5) * 0.1;
            body.friction = 0.3;
            body.shape.type = (i % 3 == 0) ? flgod::ShapeType::Box : flgod::ShapeType::Sphere;
            body.shape.radius = 0.4;
            body.shape.half_extents = flgod::Vec3(0.4, 0.4, 0.4);
            body.motion_type = flgod::BodyMotionType::Dynamic;
            body.fidelity = flgod::SimulationFidelity::L0_Full;
            engine.add_body(body);
        }

        // Add a constraint between bodies 1 and 2
        flgod::PhysicsConstraint c;
        c.id = 100;
        c.body_a = flgod::EntityID(flgod::EntityType::WorldObject, 1, 1);
        c.body_b = flgod::EntityID(flgod::EntityType::WorldObject, 1, 2);

        c.type = flgod::ConstraintType::Distance;
        c.target_distance = 1.5;
        c.break_force = 2000.0;
        engine.add_constraint(c);

        return engine;
    };

    // 1. Bit-exact determinism across 2 independent runs
    flgod::PhysicsEngine engine1 = setup_engine();
    flgod::PhysicsEngine engine2 = setup_engine();

    double dt = 0.01;
    for (int step = 0; step < 500; ++step) {
        engine1.step(dt);
        engine2.step(dt);

        uint64_t h1 = engine1.compute_physics_hash();
        uint64_t h2 = engine2.compute_physics_hash();

        if (h1 != h2) {
            std::cerr << "FAILED: Determinism divergence at step " << step 
                      << ": h1=0x" << std::hex << h1 << " != h2=0x" << h2 << std::dec << std::endl;
            return 1;
        }
    }
    std::cout << "  Passed 500 steps bit-exact determinism check. Final hash: 0x" 
              << std::hex << engine1.compute_physics_hash() << std::dec << std::endl;

    // 2. Checkpoint and crash recovery test
    flgod::PhysicsEngine reference = setup_engine();
    for (int step = 0; step < 500; ++step) {
        reference.step(dt);
    }
    uint64_t expected_hash = reference.compute_physics_hash();

    flgod::PhysicsEngine interrupted = setup_engine();
    for (int step = 0; step < 250; ++step) {
        interrupted.step(dt);
    }

    // Serialize to JSON
    nlohmann::json checkpoint = interrupted.to_json();
    std::string checkpoint_str = checkpoint.dump();

    // Recover into new engine instance
    flgod::PhysicsEngine recovered;
    nlohmann::json parsed = nlohmann::json::parse(checkpoint_str);
    recovered.from_json(parsed);

    // Verify midpoint hash matches
    if (recovered.compute_physics_hash() != interrupted.compute_physics_hash()) {
        std::cerr << "FAILED: Restored engine hash does not match before continuation!" << std::endl;
        return 1;
    }

    // Step the remaining 250 steps
    for (int step = 250; step < 500; ++step) {
        recovered.step(dt);
    }

    uint64_t recovered_final_hash = recovered.compute_physics_hash();
    if (recovered_final_hash != expected_hash) {
        std::cerr << "FAILED: Checkpoint-recovered physics run diverged from reference run! "
                  << "Expected: 0x" << std::hex << expected_hash 
                  << ", Got: 0x" << recovered_final_hash << std::dec << std::endl;
        return 1;
    }

    std::cout << "  Passed 100% crash recovery and checkpoint replay test. Hash: 0x" 
              << std::hex << recovered_final_hash << std::dec << std::endl;

    std::cout << "[TEST] test_deterministic_physics PASSED" << std::endl;
    return 0;
}
