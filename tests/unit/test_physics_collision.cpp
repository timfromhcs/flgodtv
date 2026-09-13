#include "flgod/physics/physics_engine.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_physics_collision..." << std::endl;

    // Zero gravity to isolate collision dynamics
    flgod::PhysicsEngine engine(flgod::Vec3(0.0, 0.0, 0.0));

    // Two identical spheres heading directly towards each other
    flgod::RigidBodyState bodyA;
    bodyA.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 10);
    bodyA.position = flgod::Vec3(-2.0, 5.0, 0.0);
    bodyA.linear_velocity = flgod::Vec3(2.0, 0.0, 0.0); // moving right
    bodyA.mass = 1.0;
    bodyA.restitution = 1.0; // elastic
    bodyA.friction = 0.0;
    bodyA.shape.type = flgod::ShapeType::Sphere;
    bodyA.shape.radius = 0.5;
    bodyA.motion_type = flgod::BodyMotionType::Dynamic;
    bodyA.fidelity = flgod::SimulationFidelity::L0_Full;

    flgod::RigidBodyState bodyB;
    bodyB.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 11);
    bodyB.position = flgod::Vec3(2.0, 5.0, 0.0);
    bodyB.linear_velocity = flgod::Vec3(-2.0, 0.0, 0.0); // moving left

    bodyB.mass = 1.0;
    bodyB.restitution = 1.0; // elastic
    bodyB.friction = 0.0;
    bodyB.shape.type = flgod::ShapeType::Sphere;
    bodyB.shape.radius = 0.5;
    bodyB.motion_type = flgod::BodyMotionType::Dynamic;
    bodyB.fidelity = flgod::SimulationFidelity::L0_Full;

    engine.add_body(bodyA);
    engine.add_body(bodyB);

    double dt = 0.01;
    bool collision_occurred = false;

    for (int step = 0; step < 150; ++step) {
        engine.step(dt);
        const auto& curA = engine.get_body(bodyA.id);
        const auto& curB = engine.get_body(bodyB.id);

        double dist = (curB.position - curA.position).length();
        // Spheres should never penetrate deeply below contact distance (sum of radii = 1.0)
        if (dist < 0.95) {
            std::cerr << "FAILED: Severe penetration detected! dist = " << dist << std::endl;
            return 1;
        }

        // Detect velocity reversal
        if (curA.linear_velocity.x < -1.0 && curB.linear_velocity.x > 1.0) {
            collision_occurred = true;
        }
    }

    if (!collision_occurred) {
        std::cerr << "FAILED: Collision velocity reversal was not observed" << std::endl;
        return 1;
    }
    std::cout << "  Head-on collision reversal verified." << std::endl;

    // 2. Raycast verification
    flgod::Vec3 ray_origin(-10.0, 5.0, 0.0);
    flgod::Vec3 ray_dir(1.0, 0.0, 0.0);
    auto hit = engine.raycast(ray_origin, ray_dir, 50.0);

    if (!hit.hit) {
        std::cerr << "FAILED: Raycast did not hit expected body" << std::endl;
        return 1;
    }
    std::cout << "  Raycast hit body " << hit.body_id.index() << " at dist " << hit.distance 
              << ", point (" << hit.point.x << ", " << hit.point.y << ", " << hit.point.z << ")" << std::endl;

    std::cout << "[TEST] test_physics_collision PASSED" << std::endl;
    return 0;
}
