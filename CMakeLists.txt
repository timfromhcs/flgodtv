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

    # Evolution subsystem tests
    add_executable(test_evolution_genome tests/unit/test_evolution_genome.cpp)
    target_link_libraries(test_evolution_genome PRIVATE flgod_core)
    add_test(NAME EvolutionGenomeTest COMMAND test_evolution_genome)

    add_executable(test_evolution_speciation tests/unit/test_evolution_speciation.cpp)
    target_link_libraries(test_evolution_speciation PRIVATE flgod_core)
    add_test(NAME EvolutionSpeciationTest COMMAND test_evolution_speciation)

    add_executable(test_evolution_coevolution tests/unit/test_evolution_coevolution.cpp)
    target_link_libraries(test_evolution_coevolution PRIVATE flgod_core)
    add_test(NAME EvolutionCoevolutionTest COMMAND test_evolution_coevolution)

    add_executable(test_evolution_population tests/deterministic/test_evolution_population.cpp)
    target_link_libraries(test_evolution_population PRIVATE flgod_core)
    add_test(NAME EvolutionPopulationTest COMMAND test_evolution_population)

    # Agent and colony subsystem tests
    add_executable(test_agent_lifecycle tests/unit/test_agent_lifecycle.cpp)
    target_link_libraries(test_agent_lifecycle PRIVATE flgod_core)
    add_test(NAME AgentLifecycleTest COMMAND test_agent_lifecycle)

    add_executable(test_agent_colony tests/unit/test_agent_colony.cpp)
    target_link_libraries(test_agent_colony PRIVATE flgod_core)
    add_test(NAME AgentColonyTest COMMAND test_agent_colony)

    add_executable(test_multi_agent_simulation tests/integration/test_multi_agent_simulation.cpp)
    target_link_libraries(test_multi_agent_simulation PRIVATE flgod_core)
    add_test(NAME MultiAgentSimulationTest COMMAND test_multi_agent_simulation)

    # LLM subsystem tests
    add_executable(test_llm_gguf_parser tests/unit/test_llm_gguf_parser.cpp)
    target_link_libraries(test_llm_gguf_parser PRIVATE flgod_core)
    add_test(NAME LLMGGUFParserTest COMMAND test_llm_gguf_parser)

    add_executable(test_llm_model_manager tests/unit/test_llm_model_manager.cpp)
    target_link_libraries(test_llm_model_manager PRIVATE flgod_core)
    add_test(NAME LLMModelManagerTest COMMAND test_llm_model_manager)

    add_executable(test_llm_action_security tests/unit/test_llm_action_security.cpp)
    target_link_libraries(test_llm_action_security PRIVATE flgod_core)
    add_test(NAME LLMActionSecurityTest COMMAND test_llm_action_security)

    add_executable(test_llm_god_fly tests/unit/test_llm_god_fly.cpp)
    target_link_libraries(test_llm_god_fly PRIVATE flgod_core)
    add_test(NAME LLMGodFlyTest COMMAND test_llm_god_fly)

    add_executable(test_llm_npc_system tests/unit/test_llm_npc_system.cpp)
    target_link_libraries(test_llm_npc_system PRIVATE flgod_core)
    add_test(NAME LLMNPCSystemTest COMMAND test_llm_npc_system)

    add_executable(test_llm_integration tests/llm/test_llm_integration.cpp)
    target_link_libraries(test_llm_integration PRIVATE flgod_core)
    add_test(NAME LLMIntegrationTest COMMAND test_llm_integration)

    # LLM Benchmark
    add_executable(benchmark_llm_throughput benchmarks/benchmark_llm_throughput.cpp)
    target_link_libraries(benchmark_llm_throughput PRIVATE flgod_core)

    # Language and social learning subsystem tests
    add_executable(test_language_symbol tests/unit/test_language_symbol.cpp)
    target_link_libraries(test_language_symbol PRIVATE flgod_core)
    add_test(NAME LanguageSymbolTest COMMAND test_language_symbol)

    add_executable(test_language_vocabulary tests/unit/test_language_vocabulary.cpp)
    target_link_libraries(test_language_vocabulary PRIVATE flgod_core)
    add_test(NAME LanguageVocabularyTest COMMAND test_language_vocabulary)

    add_executable(test_language_social_learning tests/unit/test_language_social_learning.cpp)
    target_link_libraries(test_language_social_learning PRIVATE flgod_core)
    add_test(NAME LanguageSocialLearningTest COMMAND test_language_social_learning)

    add_executable(test_language_transmission tests/integration/test_language_transmission.cpp)
    target_link_libraries(test_language_transmission PRIVATE flgod_core)
    add_test(NAME LanguageTransmissionTest COMMAND test_language_transmission)

    # Language Benchmark
    add_executable(benchmark_language_transmission benchmarks/benchmark_language_transmission.cpp)
    target_link_libraries(benchmark_language_transmission PRIVATE flgod_core)

    # Technology subsystem tests
    add_executable(test_technology_tools tests/unit/test_technology_tools.cpp)
    target_link_libraries(test_technology_tools PRIVATE flgod_core)
    add_test(NAME TechnologyToolsTest COMMAND test_technology_tools)

    add_executable(test_technology_vm tests/unit/test_technology_vm.cpp)
    target_link_libraries(test_technology_vm PRIVATE flgod_core)
    add_test(NAME TechnologyVMTest COMMAND test_technology_vm)

    add_executable(test_technology_network tests/unit/test_technology_network.cpp)
    target_link_libraries(test_technology_network PRIVATE flgod_core)
    add_test(NAME TechnologyNetworkTest COMMAND test_technology_network)

    add_executable(test_technology_integration tests/integration/test_technology_integration.cpp)
    target_link_libraries(test_technology_integration PRIVATE flgod_core)
    add_test(NAME TechnologyIntegrationTest COMMAND test_technology_integration)

    # Technology Benchmark
    add_executable(benchmark_technology_vm benchmarks/benchmark_technology_vm.cpp)
    target_link_libraries(benchmark_technology_vm PRIVATE flgod_core)

    # Fly-Brain adapter subsystem tests
    add_executable(test_brain_interface tests/unit/test_brain_interface.cpp)
    target_link_libraries(test_brain_interface PRIVATE flgod_core)
    add_test(NAME BrainInterfaceTest COMMAND test_brain_interface)

    add_executable(test_brain_malecns_loader tests/unit/test_brain_malecns_loader.cpp)
    target_link_libraries(test_brain_malecns_loader PRIVATE flgod_core)
    add_test(NAME BrainMaleCNSLoaderTest COMMAND test_brain_malecns_loader)

    add_executable(test_brain_agent_integration tests/integration/test_brain_agent_integration.cpp)
    target_link_libraries(test_brain_agent_integration PRIVATE flgod_core)
    add_test(NAME BrainAgentIntegrationTest COMMAND test_brain_agent_integration)

    # Fly-Brain Benchmark
    add_executable(benchmark_brain_simulation benchmarks/benchmark_brain_simulation.cpp)
    target_link_libraries(benchmark_brain_simulation PRIVATE flgod_core)

    # Multi-Agent ecosystem tests
    add_executable(test_multi_agent_ecosystem tests/integration/test_multi_agent_ecosystem.cpp)
    target_link_libraries(test_multi_agent_ecosystem PRIVATE flgod_core)
    add_test(NAME MultiAgentEcosystemTest COMMAND test_multi_agent_ecosystem)

    # Multi-Agent Benchmark
    add_executable(benchmark_multi_agent_ecosystem benchmarks/benchmark_multi_agent_ecosystem.cpp)
    target_link_libraries(benchmark_multi_agent_ecosystem PRIVATE flgod_core)
endif()


