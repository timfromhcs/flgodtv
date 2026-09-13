cmake_minimum_required(VERSION 3.20)

project(flgodtv
    VERSION 0.1.0
    DESCRIPTION "FLGODTV Autonomous Simulation Platform"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler specific options
if(MSVC)
    add_compile_options(/W4 /EHsc /utf-8 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Core include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Core library
add_library(flgod_core INTERFACE)
target_include_directories(flgod_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Main headless executable
add_executable(flgod
    src/main.cpp
)
target_link_libraries(flgod PRIVATE flgod_core)

# Testing
option(BUILD_TESTING "Build test suite" ON)
if(BUILD_TESTING)
    enable_testing()

    add_executable(test_core_clock tests/unit/test_core_clock.cpp)
    target_link_libraries(test_core_clock PRIVATE flgod_core)
    add_test(NAME CoreClockTest COMMAND test_core_clock)

    add_executable(test_core_rng tests/unit/test_core_rng.cpp)
    target_link_libraries(test_core_rng PRIVATE flgod_core)
    add_test(NAME CoreRNGTest COMMAND test_core_rng)

    add_executable(test_core_entity tests/unit/test_core_entity.cpp)
    target_link_libraries(test_core_entity PRIVATE flgod_core)
    add_test(NAME CoreEntityTest COMMAND test_core_entity)

    add_executable(test_core_event_bus tests/unit/test_core_event_bus.cpp)
    target_link_libraries(test_core_event_bus PRIVATE flgod_core)
    add_test(NAME CoreEventBusTest COMMAND test_core_event_bus)

    add_executable(test_deterministic_simulation tests/deterministic/test_deterministic_simulation.cpp)
    target_link_libraries(test_deterministic_simulation PRIVATE flgod_core)
    add_test(NAME DeterministicSimulationTest COMMAND test_deterministic_simulation)

    # World subsystem tests
    add_executable(test_world_noise tests/unit/test_world_noise.cpp)
    target_link_libraries(test_world_noise PRIVATE flgod_core)
    add_test(NAME WorldNoiseTest COMMAND test_world_noise)

    add_executable(test_world_chunk tests/unit/test_world_chunk.cpp)
    target_link_libraries(test_world_chunk PRIVATE flgod_core)
    add_test(NAME WorldChunkTest COMMAND test_world_chunk)

    add_executable(test_world_fields tests/unit/test_world_fields.cpp)
    target_link_libraries(test_world_fields PRIVATE flgod_core)
    add_test(NAME WorldFieldsTest COMMAND test_world_fields)

    add_executable(test_world_procedural tests/unit/test_world_procedural.cpp)
    target_link_libraries(test_world_procedural PRIVATE flgod_core)
    add_test(NAME WorldProceduralTest COMMAND test_world_procedural)

    add_executable(test_deterministic_world tests/deterministic/test_deterministic_world.cpp)
    target_link_libraries(test_deterministic_world PRIVATE flgod_core)
    add_test(NAME DeterministicWorldTest COMMAND test_deterministic_world)

    # Physics subsystem tests
    add_executable(test_physics_fall tests/unit/test_physics_fall.cpp)
    target_link_libraries(test_physics_fall PRIVATE flgod_core)
    add_test(NAME PhysicsFallTest COMMAND test_physics_fall)

    add_executable(test_physics_collision tests/unit/test_physics_collision.cpp)
    target_link_libraries(test_physics_collision PRIVATE flgod_core)
    add_test(NAME PhysicsCollisionTest COMMAND test_physics_collision)

    add_executable(test_physics_constraint tests/unit/test_physics_constraint.cpp)
    target_link_libraries(test_physics_constraint PRIVATE flgod_core)
    add_test(NAME PhysicsConstraintTest COMMAND test_physics_constraint)

    add_executable(test_physics_fidelity tests/unit/test_physics_fidelity.cpp)
    target_link_libraries(test_physics_fidelity PRIVATE flgod_core)
    add_test(NAME PhysicsFidelityTest COMMAND test_physics_fidelity)

    add_executable(test_deterministic_physics tests/deterministic/test_deterministic_physics.cpp)
    target_link_libraries(test_deterministic_physics PRIVATE flgod_core)
    add_test(NAME DeterministicPhysicsTest COMMAND test_deterministic_physics)
endif()

