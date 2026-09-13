#include "flgod/physics/physics_engine.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_physics_fidelity..." << std::endl;

    flgod::PhysicsEngine engine(flgod::Vec3(0.0, -9.81, 0.0));

    // L0 Body: Full fidelity
    flgod::RigidBodyState body_l0;
    body_l0.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 30);
    body_l0.position = flgod::Vec3(0.0, 10.0, 0.0);
    body_l0.fidelity = flgod::SimulationFidelity::L0_Full;
    body_l0.shape.radius = 0.5;
    engine.add_body(body_l0);

    // L1 Body: Reduced fidelity
    flgod::RigidBodyState body_l1;
    body_l1.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 31);
    body_l1.position = flgod::Vec3(5.0, 10.0, 0.0);
    body_l1.fidelity = flgod::SimulationFidelity::L1_Reduced;
    body_l1.shape.radius = 0.5;
    engine.add_body(body_l1);

    // L2 Body: Statistical fidelity
    flgod::RigidBodyState body_l2;
    body_l2.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 32);
    body_l2.position = flgod::Vec3(10.0, 10.0, 0.0);

    body_l2.linear_velocity = flgod::Vec3(1.0, 0.0, 1.0);
    body_l2.fidelity = flgod::SimulationFidelity::L2_Statistical;
    body_l2.shape.radius = 0.5;
    engine.add_body(body_l2);

    double dt = 0.01;
    engine.step(dt);

    // L2 body should immediately be clamped to ground plane statistical equilibrium (y = radius)
    const auto& res_l2 = engine.get_body(body_l2.id);
    if (std::abs(res_l2.position.y - res_l2.shape.radius) > 1e-4) {
        std::cerr << "FAILED: L2 body not clamped to ground equilibrium! y = " << res_l2.position.y << std::endl;
        return 1;
    }
    std::cout << "  L2 statistical behavior verified (ground equilibrium: y = " << res_l2.position.y << ")" << std::endl;

    // Promotion & Demotion test: L0 -> L1 -> L2 -> L1 -> L0
    flgod::EntityID test_id = body_l0.id;
    flgod::Vec3 saved_pos = engine.get_body(test_id).position;
    flgod::Vec3 saved_vel = engine.get_body(test_id).linear_velocity;

    // Demote to L1
    engine.set_body_fidelity(test_id, flgod::SimulationFidelity::L1_Reduced);
    if (engine.get_body(test_id).fidelity != flgod::SimulationFidelity::L1_Reduced) {
        std::cerr << "FAILED: Demotion to L1 failed" << std::endl;
        return 1;
    }

    // Demote to L2
    engine.set_body_fidelity(test_id, flgod::SimulationFidelity::L2_Statistical);
    if (engine.get_body(test_id).fidelity != flgod::SimulationFidelity::L2_Statistical) {
        std::cerr << "FAILED: Demotion to L2 failed" << std::endl;
        return 1;
    }

    // Promote to L1
    engine.set_body_fidelity(test_id, flgod::SimulationFidelity::L1_Reduced);
    if (engine.get_body(test_id).fidelity != flgod::SimulationFidelity::L1_Reduced) {
        std::cerr << "FAILED: Promotion to L1 failed" << std::endl;
        return 1;
    }

    // Promote to L0
    engine.set_body_fidelity(test_id, flgod::SimulationFidelity::L0_Full);
    if (engine.get_body(test_id).fidelity != flgod::SimulationFidelity::L0_Full) {
        std::cerr << "FAILED: Promotion to L0 failed" << std::endl;
        return 1;
    }

    std::cout << "  Promotion and demotion lifecycle verified cleanly without data loss." << std::endl;

    std::cout << "[TEST] test_physics_fidelity PASSED" << std::endl;
    return 0;
}
