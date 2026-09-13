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

# Vulkan dependency
find_package(Vulkan REQUIRED)

# Core library
add_library(flgod_core INTERFACE)
target_include_directories(flgod_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(flgod_core INTERFACE
    Vulkan::Vulkan
)

# Copy shaders to binary dir for tests and execution
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


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

    # Headless backend integration tests
    add_executable(test_headless_modes tests/integration/test_headless_modes.cpp)
    target_link_libraries(test_headless_modes PRIVATE flgod_core)
    add_test(NAME HeadlessModesTest COMMAND test_headless_modes)

    # Vulkan compute subsystem tests
    add_executable(test_vulkan_init tests/gpu/test_vulkan_init.cpp)
    target_link_libraries(test_vulkan_init PRIVATE flgod_core)
    add_test(NAME VulkanInitTest COMMAND test_vulkan_init)

    add_executable(test_vulkan_buffer tests/gpu/test_vulkan_buffer.cpp)
    target_link_libraries(test_vulkan_buffer PRIVATE flgod_core)
    add_test(NAME VulkanBufferTest COMMAND test_vulkan_buffer)

    add_executable(test_vulkan_compute tests/gpu/test_vulkan_compute.cpp)
    target_link_libraries(test_vulkan_compute PRIVATE flgod_core)
    add_test(NAME VulkanComputeTest COMMAND test_vulkan_compute)

    # Learning subsystem tests
    add_executable(test_learning_experience tests/unit/test_learning_experience.cpp)
    target_link_libraries(test_learning_experience PRIVATE flgod_core)
    add_test(NAME LearningExperienceTest COMMAND test_learning_experience)

    add_executable(test_learning_memory tests/unit/test_learning_memory.cpp)
    target_link_libraries(test_learning_memory PRIVATE flgod_core)
    add_test(NAME LearningMemoryTest COMMAND test_learning_memory)

    add_executable(test_learning_loop tests/unit/test_learning_loop.cpp)
    target_link_libraries(test_learning_loop PRIVATE flgod_core)
    add_test(NAME LearningLoopTest COMMAND test_learning_loop)

    add_executable(test_learning_parallel tests/integration/test_learning_parallel.cpp)
    target_link_libraries(test_learning_parallel PRIVATE flgod_core)
    add_test(NAME LearningParallelTest COMMAND test_learning_parallel)

    # Benchmarks
    add_executable(benchmark_learning_sample_efficiency benchmarks/benchmark_learning_sample_efficiency.cpp)
    target_link_libraries(benchmark_learning_sample_efficiency PRIVATE flgod_core)
endif()


