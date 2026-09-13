#include "flgod/physics/physics_engine.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_physics_fall..." << std::endl;

    flgod::PhysicsEngine engine(flgod::Vec3(0.0, -9.81, 0.0));

    // 1. Free fall test (no ground contact initially)
    flgod::RigidBodyState ball;
    ball.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 1);
    ball.position = flgod::Vec3(0.0, 50.0, 0.0);
    ball.linear_velocity = flgod::Vec3(0.0, 0.0, 0.0);
    ball.mass = 2.0;
    ball.restitution = 0.5;
    ball.friction = 0.3;
    ball.shape.type = flgod::ShapeType::Sphere;
    ball.shape.radius = 1.0;
    ball.motion_type = flgod::BodyMotionType::Dynamic;
    ball.fidelity = flgod::SimulationFidelity::L0_Full;

    engine.add_body(ball);

    double dt = 0.01;
    // Step for 1.0 second (100 steps)
    for (int i = 0; i < 100; ++i) {
        engine.step(dt);
    }

    const auto& b_after_1s = engine.get_body(ball.id);
    // Theoretical free fall with linear damping:
    // Velocity should be downward and roughly -9.81 m/s (modified slightly by damping)
    if (b_after_1s.linear_velocity.y >= 0.0) {
        std::cerr << "FAILED: Ball velocity is not negative under gravity: " << b_after_1s.linear_velocity.y << std::endl;
        return 1;
    }
    if (b_after_1s.position.y >= 50.0) {
        std::cerr << "FAILED: Ball did not fall downwards: " << b_after_1s.position.y << std::endl;
        return 1;
    }
    std::cout << "  Free fall at 1.0s: y = " << b_after_1s.position.y << ", vy = " << b_after_1s.linear_velocity.y << std::endl;

    // 2. Drop from lower height to test ground collision and restitution bounce
    flgod::RigidBodyState bouncer;
    bouncer.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 2);

    bouncer.position = flgod::Vec3(10.0, 5.0, 0.0);
    bouncer.linear_velocity = flgod::Vec3(0.0, 0.0, 0.0);
    bouncer.mass = 1.0;
    bouncer.restitution = 0.6;
    bouncer.friction = 0.2;
    bouncer.shape.type = flgod::ShapeType::Sphere;
    bouncer.shape.radius = 0.5;
    bouncer.motion_type = flgod::BodyMotionType::Dynamic;
    bouncer.fidelity = flgod::SimulationFidelity::L0_Full;

    engine.add_body(bouncer);

    bool observed_bounce = false;
    double prev_vy = 0.0;

    for (int step = 0; step < 200; ++step) {
        engine.step(dt);
        const auto& cur = engine.get_body(bouncer.id);

        // Verify ball never penetrates below ground (y < radius)
        if (cur.position.y < cur.shape.radius - 1e-4) {
            std::cerr << "FAILED: Ball penetrated below ground: y = " << cur.position.y << std::endl;
            return 1;
        }

        // Detect bounce: upward velocity immediately after downward velocity at ground
        if (prev_vy < -1.0 && cur.linear_velocity.y > 0.5 && std::abs(cur.position.y - cur.shape.radius) < 0.1) {
            observed_bounce = true;
        }
        prev_vy = cur.linear_velocity.y;
    }

    if (!observed_bounce) {
        std::cerr << "FAILED: Did not observe restitution bounce from ground" << std::endl;
        return 1;
    }
    std::cout << "  Restitution bounce observed successfully." << std::endl;

    // 3. Settling test: after extended steps, bouncer should settle to rest on ground
    for (int step = 0; step < 1000; ++step) {
        engine.step(dt);
    }
    const auto& settled = engine.get_body(bouncer.id);
    if (std::abs(settled.position.y - settled.shape.radius) > 0.01) {
        std::cerr << "FAILED: Ball did not settle at ground level: y = " << settled.position.y << std::endl;
        return 1;
    }
    if (std::abs(settled.linear_velocity.y) > 0.05) {
        std::cerr << "FAILED: Ball vertical velocity not settled: vy = " << settled.linear_velocity.y << std::endl;
        return 1;
    }
    std::cout << "  Ball settled on ground at y = " << settled.position.y << ", vy = " << settled.linear_velocity.y << std::endl;

    std::cout << "[TEST] test_physics_fall PASSED" << std::endl;
    return 0;
}
