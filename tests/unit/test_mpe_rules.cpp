#include "flgod/mpe/rules.hpp"
#include <iostream>

#define CHECK(cond, msg) do { if (!(cond)) { \
    std::cerr << "FAILED: " << msg << std::endl; return 1; } } while (0)

using namespace flgod::mpe;

static RuleContext make_ctx() {
    RuleContext ctx;
    ctx.tick = 10;
    RuleEntity a;
    a.raw = 1;
    a.archetype = "ant";
    a.energy = 40.0;
    a.x = 100.0;
    RuleEntity b;
    b.raw = 2;
    b.archetype = "ant";
    b.energy = 90.0;
    RuleEntity p;
    p.raw = 3;
    p.archetype = "predator";
    p.energy = 80.0;
    ctx.entities = {a, b, p};
    return ctx;
}

int main() {
    std::cout << "[TEST] Running test_mpe_rules..." << std::endl;
    RuleRegistry reg;
    CHECK(reg.known("foraging") && reg.known("predation") && reg.known("cooperation") &&
              reg.known("territory") && reg.known("survival") && reg.known("reproduction"),
          "built-in rules registered");
    bool unknown = false;
    try { auto unused = reg.create("terraform_mars"); (void)unused; } catch (const std::runtime_error&) { unknown = true; }
    CHECK(unknown, "unknown rule must throw");
    bool dup = false;
    try { reg.register_rule("foraging", []() { return std::make_unique<ForagingRule>(); }); }
    catch (const std::runtime_error&) { dup = true; }
    CHECK(dup, "duplicate rule registration must throw");

    // Foraging feeds the hungry only, deterministically.
    {
        RuleContext ctx = make_ctx();
        auto r = reg.create("foraging");
        uint64_t n = r->evaluate(ctx, nlohmann::json::object());
        CHECK(n == 1, "foraging fires once");
        CHECK(ctx.entities[0].energy == 42.0, "hungry entity fed");
        CHECK(ctx.entities[1].energy == 90.0, "satiated untouched");
        RuleContext ctx2 = make_ctx();
        CHECK(r->evaluate(ctx2, nlohmann::json::object()) == 1, "foraging deterministic");
    }
    // Predation transfers weakest prey energy to predator.
    {
        RuleContext ctx = make_ctx();
        auto r = reg.create("predation");
        uint64_t n = r->evaluate(ctx, nlohmann::json::object());
        CHECK(n == 1, "predation fires once");
        CHECK(ctx.entities[0].energy == 35.0, "weakest prey drained");
        CHECK(ctx.entities[2].energy == 84.0, "predator gains");
    }
    // Territory clamps radius.
    {
        RuleContext ctx = make_ctx();
        auto r = reg.create("territory");
        uint64_t n = r->evaluate(ctx, {{"max_radius", 40.0}});
        CHECK(n == 1, "territory fires once");
        CHECK(ctx.entities[0].x == 40.0, "entity clamped to radius");
        CHECK(!ctx.events.empty() && ctx.events[0].first == "TerritoryEnforced", "event emitted");
    }
    // Cooperation pulls toward group mean.
    {
        RuleContext ctx = make_ctx();
        auto r = reg.create("cooperation");
        r->evaluate(ctx, nlohmann::json::object());
        CHECK(ctx.entities[0].energy > 40.0 && ctx.entities[1].energy < 90.0, "cooperation shares");
    }
    // Survival drains critical entities.
    {
        RuleContext ctx = make_ctx();
        ctx.entities[0].energy = 5.0;
        auto r = reg.create("survival");
        CHECK(r->evaluate(ctx, nlohmann::json::object()) == 1, "survival fires on critical");
        CHECK(ctx.entities[0].energy < 5.0, "critical drained");
    }
    // Reproduction requests births with cooldown + serialize roundtrip.
    {
        RuleContext ctx = make_ctx();
        ctx.tick = 25; // past the default 20-tick cooldown (last=0)
        ctx.entities[0].energy = 99.0;
        auto r = reg.create("reproduction");
        CHECK(r->evaluate(ctx, nlohmann::json::object()) == 1, "birth requested");
        CHECK(r->evaluate(ctx, nlohmann::json::object()) == 0, "cooldown suppresses");
        nlohmann::json st = r->to_json();
        auto r2 = reg.create("reproduction");
        r2->from_json(st);
        CHECK(r2->evaluate(ctx, nlohmann::json::object()) == 0, "cooldown survives serialize");
    }

    // Mutation perturbs existing traits deterministically, skips missing ones.
    {
        RuleContext ctx = make_ctx();
        ctx.entities[0].traits["vigor"] = 0.5;
        auto r = reg.create("mutation");
        nlohmann::json cfg = {{"trait", "vigor"}, {"probability", 1.0}, {"scale", 0.1}};
        CHECK(r->evaluate(ctx, cfg) == 1, "mutation fires at p=1");
        double v = ctx.entities[0].traits["vigor"];
        CHECK(v >= 0.4 && v <= 0.6 && v != 0.5, "vigor perturbed within scale");
        RuleContext ctx2 = make_ctx();
        ctx2.entities[0].traits["vigor"] = 0.5;
        CHECK(reg.create("mutation")->evaluate(ctx2, cfg) == 1, "mutation fires again");
        CHECK(ctx2.entities[0].traits["vigor"] == v, "mutation deterministic");
        RuleContext ctx3 = make_ctx(); // no vigor trait anywhere
        CHECK(reg.create("mutation")->evaluate(ctx3, cfg) == 0, "missing trait skipped");
    }

    std::cout << "[PASS] test_mpe_rules passed successfully." << std::endl;
    return 0;
}
