#include "flgod/physics/physics_engine.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_physics_constraint..." << std::endl;

    flgod::PhysicsEngine engine(flgod::Vec3(0.0, -9.81, 0.0));

    // Anchor body (static) at (0, 10, 0)
    flgod::RigidBodyState anchor;
    anchor.id = flgod::EntityID(flgod::EntityType::Structure, 1, 20);
    anchor.position = flgod::Vec3(0.0, 10.0, 0.0);
    anchor.mass = 0.0;
    anchor.motion_type = flgod::BodyMotionType::Static;
    anchor.fidelity = flgod::SimulationFidelity::L0_Full;
    engine.add_body(anchor);

    // Hanging weight (dynamic) at (0, 8, 0), distance = 2.0
    flgod::RigidBodyState weight;
    weight.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 21);
    weight.position = flgod::Vec3(0.0, 8.0, 0.0);

    weight.mass = 1.0;
    weight.motion_type = flgod::BodyMotionType::Dynamic;
    weight.fidelity = flgod::SimulationFidelity::L0_Full;
    engine.add_body(weight);

    // Add distance constraint
    flgod::PhysicsConstraint link;
    link.id = 1;
    link.body_a = anchor.id;
    link.body_b = weight.id;
    link.type = flgod::ConstraintType::Distance;
    link.target_distance = 2.0;
    link.break_force = 500.0; // Break threshold
    link.is_broken = false;
    engine.add_constraint(link);

    double dt = 0.01;
    // Step for 1 second under gravity
    for (int step = 0; step < 100; ++step) {
        engine.step(dt);
    }

    // Check constraint holds
    const auto& bAnchor = engine.get_body(anchor.id);
    const auto& bWeight = engine.get_body(weight.id);
    double cur_dist = (bWeight.position - bAnchor.position).length();

    if (engine.constraints()[0].is_broken) {
        std::cerr << "FAILED: Constraint broke prematurely under regular gravity" << std::endl;
        return 1;
    }
    if (std::abs(cur_dist - 2.0) > 0.1) {
        std::cerr << "FAILED: Constraint distance violated: " << cur_dist << " (target 2.0)" << std::endl;
        return 1;
    }
    std::cout << "  Constraint maintained distance under gravity: cur_dist = " << cur_dist << std::endl;

    // 2. Structural failure test: Apply enormous impulse downward exceeding break_force
    engine.apply_impulse(weight.id, flgod::Vec3(0.0, -1000.0, 0.0));
    engine.step(dt);

    if (!engine.constraints()[0].is_broken) {
        std::cerr << "FAILED: Constraint failed to break under extreme force!" << std::endl;
        return 1;
    }
    std::cout << "  Structural failure successfully triggered when force exceeded threshold." << std::endl;

    // Further steps: with constraint broken, weight falls freely
    engine.step(dt);
    double dist_after_break = (engine.get_body(weight.id).position - engine.get_body(anchor.id).position).length();
    if (dist_after_break <= cur_dist) {
        std::cerr << "FAILED: Body did not separate after structural failure!" << std::endl;
        return 1;
    }
    std::cout << "  Bodies separated after failure: dist = " << dist_after_break << std::endl;

    std::cout << "[TEST] test_physics_constraint PASSED" << std::endl;
    return 0;
}
