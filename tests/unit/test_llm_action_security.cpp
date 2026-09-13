#include <cassert>
#include <iostream>
#include "flgod/llm/action_security.hpp"
#include "flgod/agents/agent_manager.hpp"

int main() {
    std::cout << "[Test] Running Action Security Unit Tests...\n";

    flgod::llm::ActionSecurityValidator validator;
    flgod::EntityID god_id(999);
    flgod::EntityID npc_id(101);

    // 1. Stage 1 Test: Malformed JSON syntax
    std::string malformed_json = "{ action: \"speech\", unclosed: ";
    auto res_malformed = validator.validate_and_parse(malformed_json, npc_id, false);
    assert(!res_malformed.passed);
    assert(res_malformed.failed_stage == flgod::llm::SecurityStage::Parse);
    std::cout << "  Passed Malformed JSON rejection test: " << res_malformed.error_reason << "\n";

    // 2. Stage 1 Test: Markdown code fence stripping
    std::string fenced_json = "```json\n{\n  \"action\": \"speech\",\n  \"parameters\": {\"content\": \"Hello world\"}\n}\n```";
    auto res_fenced = validator.validate_and_parse(fenced_json, npc_id, false);
    assert(res_fenced.passed && "Failed to parse fenced JSON");
    assert(res_fenced.action.text_content == "Hello world");

    // 3. Stage 2 Test: Sandbox escape attempt / forbidden keys
    std::string exploit_json = "{\n  \"action\": \"speech\",\n  \"parameters\": {\n    \"content\": \"Normal\",\n    \"exec\": \"rm -rf /\"\n  }\n}";
    auto res_exploit = validator.validate_and_parse(exploit_json, npc_id, false);
    assert(!res_exploit.passed);
    assert(res_exploit.failed_stage == flgod::llm::SecurityStage::SchemaValidation);
    std::cout << "  Passed Sandbox Escape rejection test: " << res_exploit.error_reason << "\n";

    // 4. Stage 3 Test: Capability Validation
    std::string unauthorized_teach = "{\n  \"action\": \"concept\",\n  \"parameters\": {\n    \"concept_id\": \"foraging_advanced\"\n  }\n}";
    auto res_unauthorized = validator.validate_and_parse(unauthorized_teach, npc_id, /*is_god_fly=*/false);
    assert(!res_unauthorized.passed);
    assert(res_unauthorized.failed_stage == flgod::llm::SecurityStage::CapabilityValidation);
    std::cout << "  Passed Capability Validation rejection test: " << res_unauthorized.error_reason << "\n";

    auto res_authorized = validator.validate_and_parse(unauthorized_teach, god_id, /*is_god_fly=*/true);
    assert(res_authorized.passed);
    assert(res_authorized.action.type == flgod::llm::ValidatedActionType::Concept);

    // 5. Stage 4 Test: World Validation with AgentManager
    flgod::AgentManager agent_mgr;
    flgod::EntityID student_id(flgod::EntityType::Agent, 1, 501);
    flgod::Agent student_agent(student_id);
    student_agent.set_position({10.0, 0.0, 0.0});
    agent_mgr.spawn_agent(student_agent);

    // Valid world target
    std::string valid_target_json = "{\n  \"action\": \"concept\",\n  \"parameters\": {\n    \"concept_id\": \"foraging_flowers\",\n    \"target_agent\": " + std::to_string(student_id.raw()) + "\n  }\n}";
    auto res_valid_target = validator.validate_and_parse(valid_target_json, god_id, true, &agent_mgr);
    assert(res_valid_target.passed);
    assert(res_valid_target.action.target_agent_id == student_id);

    // Non-existent target
    std::string invalid_target_json = "{\n  \"action\": \"concept\",\n  \"parameters\": {\n    \"concept_id\": \"foraging_flowers\",\n    \"target_agent\": 99999999\n  }\n}";
    auto res_invalid_target = validator.validate_and_parse(invalid_target_json, god_id, true, &agent_mgr);
    assert(!res_invalid_target.passed);
    assert(res_invalid_target.failed_stage == flgod::llm::SecurityStage::WorldValidation);
    std::cout << "  Passed Non-existent Target rejection test: " << res_invalid_target.error_reason << "\n";

    // Target dead
    agent_mgr.get_agent(student_id).kill();
    auto res_dead_target = validator.validate_and_parse(valid_target_json, god_id, true, &agent_mgr);
    assert(!res_dead_target.passed);
    assert(res_dead_target.failed_stage == flgod::llm::SecurityStage::WorldValidation);
    std::cout << "  Passed Dead Target rejection test: " << res_dead_target.error_reason << "\n";

    std::cout << "[Test] Action Security Unit Tests PASSED!\n";
    return 0;
}
