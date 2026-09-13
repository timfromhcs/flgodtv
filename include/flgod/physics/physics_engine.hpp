#pragma once

#include "flgod/physics/physics_types.hpp"
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace flgod {

class PhysicsEngine {
public:
    explicit PhysicsEngine(Vec3 gravity = Vec3(0.0, -9.81, 0.0))
        : m_gravity(gravity) {}

    void set_gravity(const Vec3& g) noexcept { m_gravity = g; }
    [[nodiscard]] const Vec3& gravity() const noexcept { return m_gravity; }

    void add_body(const RigidBodyState& body) {
        m_bodies[body.id] = body;
    }

    void remove_body(EntityID id) {
        m_bodies.erase(id);
    }

    [[nodiscard]] bool has_body(EntityID id) const noexcept {
        return m_bodies.find(id) != m_bodies.end();
    }

    [[nodiscard]] const RigidBodyState& get_body(EntityID id) const {
        auto it = m_bodies.find(id);
        if (it == m_bodies.end()) {
            throw std::runtime_error("Body not found in physics engine");
        }
        return it->second;
    }

    RigidBodyState& get_body(EntityID id) {
        auto it = m_bodies.find(id);
        if (it == m_bodies.end()) {
            throw std::runtime_error("Body not found in physics engine");
        }
        return it->second;
    }

    void set_body_position(EntityID id, const Vec3& pos) {
        get_body(id).position = pos;
    }

    void set_body_velocity(EntityID id, const Vec3& vel) {
        get_body(id).linear_velocity = vel;
    }

    void apply_impulse(EntityID id, const Vec3& impulse) {
        auto& b = get_body(id);
        if (b.motion_type == BodyMotionType::Dynamic && b.mass > 0.0) {
            b.linear_velocity = b.linear_velocity + impulse * (1.0 / b.mass);
        }
    }

    void apply_force(EntityID id, const Vec3& force, double dt) {
        apply_impulse(id, force * dt);
    }

    void add_constraint(const PhysicsConstraint& constraint) {
        m_constraints.push_back(constraint);
    }

    [[nodiscard]] const std::vector<PhysicsConstraint>& constraints() const noexcept {
        return m_constraints;
    }

    void set_body_fidelity(EntityID id, SimulationFidelity fidelity) {
        get_body(id).fidelity = fidelity;
    }

    void step(double dt) {
        if (dt <= 0.0) return;

        // Collect and sort body IDs for bit-exact deterministic execution
        std::vector<EntityID> sorted_body_ids;
        sorted_body_ids.reserve(m_bodies.size());
        for (const auto& [id, _] : m_bodies) sorted_body_ids.push_back(id);
        std::sort(sorted_body_ids.begin(), sorted_body_ids.end());

        // 1. Multi-fidelity velocity integration
        for (EntityID id : sorted_body_ids) {
            auto& body = m_bodies.at(id);
            if (!body.is_active || body.motion_type != BodyMotionType::Dynamic) {
                continue;
            }

            if (body.fidelity == SimulationFidelity::L0_Full) {
                // Apply gravity and damping
                Vec3 g_force = m_gravity * (body.gravity_factor * dt);
                body.linear_velocity = body.linear_velocity + g_force;
                body.linear_velocity = body.linear_velocity * (1.0 - body.linear_damping * dt);

                // Continuous collision detection check before step
                if (body.ccd_enabled) {
                    double speed = body.linear_velocity.length();
                    double max_step = body.shape.radius;
                    if (speed * dt > max_step) {
                        // Substep integration for high-speed CCD
                        int substeps = static_cast<int>(std::ceil(speed * dt / max_step));
                        substeps = std::min(substeps, 8);
                        double sub_dt = dt / substeps;
                        for (int s = 0; s < substeps; ++s) {
                            body.position = body.position + body.linear_velocity * sub_dt;
                            resolve_ground_collision(body);
                        }
                        continue;
                    }
                }
                body.position = body.position + body.linear_velocity * dt;
                resolve_ground_collision(body);

            } else if (body.fidelity == SimulationFidelity::L1_Reduced) {
                // Reduced fidelity: simplified gravity + drag, ground collision only
                body.linear_velocity.y += m_gravity.y * dt;
                body.linear_velocity = body.linear_velocity * (1.0 - 0.1 * dt);
                body.position = body.position + body.linear_velocity * dt;
                resolve_ground_collision(body);

            } else if (body.fidelity == SimulationFidelity::L2_Statistical) {
                // Statistical fidelity: statistical equilibrium on ground plane
                body.linear_velocity = body.linear_velocity * 0.95;
                body.position.x += body.linear_velocity.x * dt;
                body.position.z += body.linear_velocity.z * dt;
                body.position.y = std::max(0.0, body.shape.radius); // clamped to ground
            }
        }

        // 2. Inter-body collision resolution for L0 bodies (strictly ordered)
        std::vector<EntityID> active_l0;
        for (EntityID id : sorted_body_ids) {
            const auto& body = m_bodies.at(id);
            if (body.is_active && body.fidelity == SimulationFidelity::L0_Full) {
                active_l0.push_back(id);
            }
        }

        size_t n = active_l0.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                resolve_pair_collision(m_bodies[active_l0[i]], m_bodies[active_l0[j]]);
            }
        }

        // 3. Constraint resolution and structural failure detection
        for (auto& c : m_constraints) {
            if (c.is_broken) continue;
            auto itA = m_bodies.find(c.body_a);
            auto itB = m_bodies.find(c.body_b);
            if (itA == m_bodies.end() || itB == m_bodies.end()) continue;

            RigidBodyState& bA = itA->second;
            RigidBodyState& bB = itB->second;

            Vec3 delta = bB.position - bA.position;
            double cur_dist = delta.length();
            if (cur_dist < 1e-6) continue;

            double diff = cur_dist - c.target_distance;
            Vec3 dir = delta * (1.0 / cur_dist);

            // Calculate internal constraint tension force: Hooke's approximation F = k * x
            double k = 500.0;
            double force_magnitude = std::abs(diff) * k;

            if (force_magnitude > c.break_force) {
                c.is_broken = true; // Structural failure!
                continue;
            }

            // Relaxation impulse
            double total_inv_mass = 0.0;
            if (bA.motion_type == BodyMotionType::Dynamic && bA.mass > 0.0) total_inv_mass += 1.0 / bA.mass;
            if (bB.motion_type == BodyMotionType::Dynamic && bB.mass > 0.0) total_inv_mass += 1.0 / bB.mass;

            if (total_inv_mass > 1e-6) {
                double corr = diff / total_inv_mass;
                if (bA.motion_type == BodyMotionType::Dynamic) {
                    bA.position = bA.position + dir * (corr * (1.0 / bA.mass) * 0.5);
                }
                if (bB.motion_type == BodyMotionType::Dynamic) {
                    bB.position = bB.position - dir * (corr * (1.0 / bB.mass) * 0.5);
                }
            }
        }
    }

    [[nodiscard]] RaycastHit raycast(const Vec3& origin, const Vec3& direction, double max_dist) const {
        RaycastHit hit{};
        hit.distance = max_dist;
        double dir_len = direction.length();
        if (dir_len < 1e-6) return hit;
        Vec3 dir_norm = direction * (1.0 / dir_len);

        for (const auto& [id, body] : m_bodies) {
            if (!body.is_active) continue;

            if (body.shape.type == ShapeType::Sphere) {
                Vec3 oc = origin - body.position;
                double b = oc.x * dir_norm.x + oc.y * dir_norm.y + oc.z * dir_norm.z;
                double c = (oc.x * oc.x + oc.y * oc.y + oc.z * oc.z) - body.shape.radius * body.shape.radius;
                double disc = b * b - c;
                if (disc >= 0.0) {
                    double t = -b - std::sqrt(disc);
                    if (t > 0.0 && t < hit.distance) {
                        hit.hit = true;
                        hit.body_id = id;
                        hit.distance = t;
                        hit.point = origin + dir_norm * t;
                        hit.normal = (hit.point - body.position) * (1.0 / body.shape.radius);
                    }
                }
            }
        }
        return hit;
    }

    [[nodiscard]] uint64_t compute_physics_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        // Sort IDs for deterministic order
        std::vector<uint64_t> ids;
        ids.reserve(m_bodies.size());
        for (const auto& [id, _] : m_bodies) {
            ids.push_back(id.raw());
        }
        std::sort(ids.begin(), ids.end());

        for (uint64_t raw_id : ids) {
            h ^= m_bodies.at(EntityID(raw_id)).compute_hash();
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["gravity"] = {{"x", m_gravity.x}, {"y", m_gravity.y}, {"z", m_gravity.z}};
        nlohmann::json bodies_arr = nlohmann::json::array();
        for (const auto& [_, b] : m_bodies) {
            bodies_arr.push_back(b.to_json());
        }
        j["bodies"] = bodies_arr;
        nlohmann::json constr_arr = nlohmann::json::array();
        for (const auto& c : m_constraints) {
            constr_arr.push_back(c.to_json());
        }
        j["constraints"] = constr_arr;
        j["hash"] = compute_physics_hash();
        return j;
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("gravity")) {
            m_gravity.x = j["gravity"].value("x", 0.0);
            m_gravity.y = j["gravity"].value("y", -9.81);
            m_gravity.z = j["gravity"].value("z", 0.0);
        }
        m_bodies.clear();
        if (j.contains("bodies")) {
            for (const auto& bj : j["bodies"]) {
                RigidBodyState b;
                b.from_json(bj);
                m_bodies[b.id] = b;
            }
        }
        m_constraints.clear();
        if (j.contains("constraints")) {
            for (const auto& cj : j["constraints"]) {
                PhysicsConstraint c;
                c.from_json(cj);
                m_constraints.push_back(c);
            }
        }
    }

private:
    void resolve_ground_collision(RigidBodyState& b) {
        double floor_y = 0.0;
        double radius = (b.shape.type == ShapeType::Box) ? b.shape.half_extents.y : b.shape.radius;
        double min_y = floor_y + radius;

        if (b.position.y < min_y) {
            b.position.y = min_y;
            if (b.linear_velocity.y < 0.0) {
                // Inelastic or restitution bounce
                b.linear_velocity.y = -b.linear_velocity.y * b.restitution;
                // Ground friction
                b.linear_velocity.x *= (1.0 - b.friction * 0.1);
                b.linear_velocity.z *= (1.0 - b.friction * 0.1);
                if (std::abs(b.linear_velocity.y) < 0.25) {
                    b.linear_velocity.y = 0.0; // Settle
                }
            }
        }
    }

    void resolve_pair_collision(RigidBodyState& a, RigidBodyState& b) {
        double radA = (a.shape.type == ShapeType::Box) ? a.shape.half_extents.length() : a.shape.radius;
        double radB = (b.shape.type == ShapeType::Box) ? b.shape.half_extents.length() : b.shape.radius;
        double target_dist = radA + radB;

        Vec3 delta = b.position - a.position;
        double dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

        if (dist_sq < target_dist * target_dist && dist_sq > 1e-8) {
            double dist = std::sqrt(dist_sq);
            Vec3 normal = delta * (1.0 / dist);
            double penetration = target_dist - dist;

            // Separation resolution
            double total_inv_mass = 0.0;
            if (a.motion_type == BodyMotionType::Dynamic && a.mass > 0.0) total_inv_mass += 1.0 / a.mass;
            if (b.motion_type == BodyMotionType::Dynamic && b.mass > 0.0) total_inv_mass += 1.0 / b.mass;
            if (total_inv_mass < 1e-6) return;

            if (a.motion_type == BodyMotionType::Dynamic) {
                a.position = a.position - normal * (penetration * (1.0 / a.mass) / total_inv_mass);
            }
            if (b.motion_type == BodyMotionType::Dynamic) {
                b.position = b.position + normal * (penetration * (1.0 / b.mass) / total_inv_mass);
            }

            // Normal impulse
            Vec3 rel_vel = b.linear_velocity - a.linear_velocity;
            double vel_along_norm = rel_vel.x * normal.x + rel_vel.y * normal.y + rel_vel.z * normal.z;
            if (vel_along_norm < 0.0) {
                double e = std::min(a.restitution, b.restitution);
                double j = -(1.0 + e) * vel_along_norm / total_inv_mass;
                Vec3 impulse = normal * j;

                if (a.motion_type == BodyMotionType::Dynamic) {
                    a.linear_velocity = a.linear_velocity - impulse * (1.0 / a.mass);
                }
                if (b.motion_type == BodyMotionType::Dynamic) {
                    b.linear_velocity = b.linear_velocity + impulse * (1.0 / b.mass);
                }
            }
        }
    }

    Vec3 m_gravity;
    std::unordered_map<EntityID, RigidBodyState> m_bodies;
    std::vector<PhysicsConstraint> m_constraints;
};

} // namespace flgod
