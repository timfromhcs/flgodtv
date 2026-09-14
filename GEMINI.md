# FLGODTV — Autonomous Engineering & Verification Contract

## 0. Mission

Build FLGODTV from the currently available starting material into a complete, reproducible, local-first, headless-capable simulation platform with:

- connectome/fly-brain integration as an external dependency
- CPU simulation backend
- Vulkan compute backend
- real-time physics
- procedural continuous environments
- weather, water, vegetation, ecology and world-state feedback
- learning engine
- memory and experience systems
- evolution and speciation systems
- body/brain co-evolution
- multiple colonies and agents
- one God Fly with local GGUF LLM teacher capability
- local NPC language models
- social learning and emergent communication
- tool use and construction
- sandboxed programmable technology layer
- Godot Vulkan frontend
- cinematic autonomous camera system
- four-camera FLGODTV presentation
- automated best-of event detection
- automated cinematic rendering
- replay/checkpoint/recovery
- deterministic experiments
- Windows and Linux builds
- local validation
- cloud CI validation
- evidence-backed documentation

The final system must be real, executable, testable and reproducible.

Do not create a fake prototype that only looks complete.

Do not write marketing claims that are not supported by executed evidence.

Do not silently skip broken components.

Do not claim a test passed unless the test actually ran and produced a verifiable result.

---

# 1. CORE PRINCIPLE

The project is built in two major phases:

```text
PHASE A
BACKEND / SIMULATION / INFRASTRUCTURE
        ↓
VERIFY COMPLETELY
        ↓
PHASE B
FRONTEND / RENDERING / CINEMATICS / UI
        ↓
VERIFY COMPLETELY
        ↓
PHASE C
INTEGRATION
        ↓
VERIFY COMPLETELY
        ↓
PHASE D
LOCAL WINDOWS + LINUX
        ↓
VERIFY COMPLETELY
        ↓
PHASE E
GITHUB + CLOUD CI
        ↓
VERIFY COMPLETELY
        ↓
PHASE F
DOCUMENTATION / README
        ↓
VERIFY CLAIMS AGAINST EVIDENCE
```

Never build the visual frontend first and then pretend the backend exists.

Never make the backend dependent on the frontend.

Never make training dependent on rendering.

Never make documentation claim functionality that has not been demonstrated.

---

# 2. NO-HALLUCINATION POLICY

Before using any external dependency, library, engine, addon, API, CLI option, model, repository or framework:

1. Inspect the real upstream source.
2. Inspect its current documentation.
3. Determine the actual installed/versioned interface.
4. Determine whether the requested feature really exists.
5. Build or install it.
6. Execute a minimal real smoke test.
7. Record the exact version/commit.
8. Only then integrate it.

Never invent:

- APIs
- classes
- functions
- CLI parameters
- files
- environment variables
- undocumented features
- plugin compatibility
- model capabilities
- benchmark results
- performance numbers
- platform support
- cloud support
- test results

If uncertain:

```text
UNKNOWN — REQUIRES VERIFICATION
```

is always preferable to guessing.

---

# 3. SOURCE OF TRUTH

The following hierarchy is mandatory:

```text
1. executable source code
2. executed test output
3. official upstream documentation
4. official upstream repository metadata
5. generated build artifacts
6. experiment manifests
7. README / prose
```

Prose is never stronger evidence than execution.

A README cannot prove a feature works.

A passing test or generated artifact can.

---

# 4. NO FAKE TESTS

Never create tests whose only purpose is to make CI green.

Forbidden:

```text
assert True
```

Forbidden:

```text
pass
```

as a replacement for actual behavior.

Forbidden:

```text
TODO
```

inside a supposedly complete implementation.

Forbidden:

```text
skip()
xfail()
expected_failure
```

unless the test is explicitly justified, documented, and is not presented as successful functionality.

Never silently disable test suites.

Never reduce test coverage merely to obtain green CI.

Never make a test weaker because implementation is difficult.

If something genuinely cannot yet be tested:

```text
UNVERIFIED
```

must be reported.

---

# 5. NO TEST SKIPPING

A failure must trigger:

```text
inspect
→ reproduce
→ diagnose
→ fix
→ rebuild
→ rerun
```

Repeat until:

```text
PASS
```

or until the agent reaches a genuine external blocker that cannot be resolved locally.

An external blocker must be documented with:

- exact error
- exact command
- exact environment
- attempted fixes
- upstream evidence
- remaining limitation

Never convert:

```text
FAIL
```

into:

```text
PASS
```

by disabling the test.

---

# 6. DETERMINISTIC ENGINEERING LOOP

Every development stage follows this exact loop:

```text
INSPECT
↓
PLAN
↓
IMPLEMENT
↓
FORMAT
↓
STATIC CHECK
↓
BUILD
↓
UNIT TEST
↓
INTEGRATION TEST
↓
REAL EXECUTION
↓
VERIFY OUTPUT
↓
DEBUG FAILURES
↓
REBUILD
↓
RERUN
↓
REPEAT UNTIL GREEN
↓
CREATE EVIDENCE
↓
ONLY THEN PROCEED
```

Do not proceed to the next major stage while the current stage is knowingly broken.

---

# 7. INITIAL REPOSITORY AUDIT

The starting directory may contain the existing fly-brain/source material.

First action:

```text
DO NOT MODIFY THE FLY-BRAIN
```

Audit everything.

Inspect:

```text
files
directories
git status
git remotes
build files
licenses
README files
source
tests
scripts
dependencies
configuration
assets
model files
```

Determine:

```text
What actually exists?
What actually builds?
What is missing?
What is assumed?
What is documented?
What is unverified?
```

Create:

```text
docs/AUDIT.md
```

with factual findings.

Every statement must be traceable to:

- source
- command output
- official documentation

---

# 8. PROTECT THE EXISTING FLY-BRAIN

The fly-brain is an external baseline.

Do not rewrite it during bootstrap.

Create an adapter boundary:

```text
FLGOD CORE
    ↓
FlyBrain Interface
    ↓
Existing Fly-Brain
```

The Fly-Brain adapter should be isolated from:

```text
world
physics
learning
evolution
rendering
LLM
```

This allows the rest of the framework to be developed and tested independently.

---

# 9. REPOSITORY ARCHITECTURE

Create:

```text
flgodtv/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── GEMINI.md
├── LICENSES/
│
├── src/
│   ├── core/
│   ├── world/
│   ├── physics/
│   ├── agents/
│   ├── brain/
│   ├── learning/
│   ├── evolution/
│   ├── language/
│   ├── llm/
│   ├── technology/
│   ├── gpu/
│   ├── persistence/
│   ├── networking/
│   ├── telemetry/
│   └── experiments/
│
├── include/
│
├── shaders/
│
├── tools/
│
├── scripts/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── deterministic/
│   ├── physics/
│   ├── gpu/
│   ├── learning/
│   ├── evolution/
│   ├── llm/
│   ├── frontend/
│   └── end_to_end/
│
├── benchmarks/
│
├── data/
│   ├── raw/
│   ├── normalized/
│   ├── cache/
│   └── manifests/
│
├── models/
│
├── checkpoints/
│
├── experiments/
│
├── evidence/
│
├── renders/
│
├── videos/
│
├── godot/
│
├── third_party/
│
└── docs/
```

---

# 10. DEPENDENCY POLICY

Every dependency must have:

```text
name
source URL
version or commit
license
platform support
build method
purpose
verification command
```

Record in:

```text
docs/DEPENDENCIES.md
```

and machine-readable:

```text
dependencies.lock.json
```

Never depend on an unpinned moving Git branch when reproducibility matters.

---

# 11. TOOLCHAIN BOOTSTRAP

Verify and/or install:

```text
Git
Git LFS
CMake
Ninja
C++ compiler
Windows SDK
Python
Vulkan SDK
Godot
Blender
FFmpeg
```

Do not assume installed versions.

Run actual detection.

Store results:

```text
evidence/toolchain/
```

Example:

```text
compiler.txt
cmake.txt
ninja.txt
python.txt
vulkaninfo.txt
godot-version.txt
ffmpeg-version.txt
blender-version.txt
```

---

# 12. LOCAL-FIRST RULE

Everything possible must work locally first.

Required order:

```text
LOCAL SOURCE
↓
LOCAL BUILD
↓
LOCAL TEST
↓
LOCAL EXECUTION
↓
LOCAL PACKAGE
↓
CLOUD
```

Cloud CI is not the development environment.

Cloud CI validates the local project.

---

# 13. BACKEND FIRST

No frontend work until the backend foundation has passed its gates.

Backend order:

```text
Core
↓
Deterministic clock
↓
RNG
↓
World state
↓
Object system
↓
Physics
↓
Headless simulation
↓
Persistence
↓
GPU backend
↓
Learning
↓
Evolution
↓
Agents
↓
Language
↓
LLM
↓
Technology
↓
Fly-brain adapter
↓
Multi-agent integration
```

---

# 14. CORE ENGINE

Implement:

```text
Simulation
SimulationClock
SimulationStep
SimulationVersion
DeterministicRNG
EntityID
EventBus
WorldState
```

Requirements:

- deterministic
- serializable
- testable
- no hidden global state
- explicit initialization
- explicit shutdown

---

# 15. DETERMINISTIC RNG

All randomness must originate from explicit seeds.

Required independent streams:

```text
world_seed
weather_seed
physics_seed
agent_seed
genome_seed
event_seed
learning_seed
render_seed
```

Do not call uncontrolled global randomness.

Test:

```text
same seed
+
same version
+
same input
=
same resulting state
```

---

# 16. WORLD STATE

The World State must be independent from Godot.

Represent:

```text
terrain
weather
water
vegetation
resources
objects
population
events
time
```

in serializable backend structures.

Godot receives state.

Godot does not own canonical simulation state.

---

# 17. WORLD CHUNKS

Implement:

```text
WorldChunk
ChunkID
ChunkSeed
ChunkState
ChunkDelta
```

Chunk generation must be deterministic:

```text
world_seed
+
chunk coordinates
+
generator version
→
chunk
```

Persist only dynamic deltas where possible.

---

# 18. PROCEDURAL ENVIRONMENT

Implement generation stages:

```text
seed
→ terrain
→ erosion
→ hydrology
→ climate
→ biomes
→ soil
→ vegetation
→ resources
→ fauna
→ points of interest
```

Every generator must have a test.

Do not use visual randomness as proof of deterministic generation.

Compare serialized results/hashes.

---

# 19. PHYSICS ENGINE

Use Jolt where appropriate for real-time rigid-body physics.

Wrap it:

```text
FLGOD Physics API
        ↓
Jolt adapter
```

Do not scatter direct Jolt calls throughout the entire codebase.

Required initial physics features:

```text
gravity
collision
rigid bodies
static bodies
constraints
friction
impulses
continuous collision detection where needed
```

---

# 20. PHYSICS TESTS

Test actual physical outcomes.

Examples:

```text
body falls under gravity
objects collide
objects do not pass through each other
constraint remains valid
object responds to impulse
structure can fail
```

Do not test implementation details when physical behavior is the real requirement.

---

# 21. MULTI-FIDELITY PHYSICS

Implement explicit simulation levels:

```text
L0 = full
L1 = reduced
L2 = statistical
```

Promotion:

```text
L2 → L1 → L0
```

Demotion:

```text
L0 → L1 → L2
```

Transitions must preserve state as closely as defined by the fidelity contract.

Never silently change state because an entity leaves the camera view.

---

# 22. WORLD FIELDS

Implement backend field systems:

```text
WindField
TemperatureField
HumidityField
MoistureField
WaterField
FireField
EcologyField
```

Fields must have:

```text
spatial state
time dependence
sampling interface
serialization
test fixtures
```

---

# 23. ECOLOGY

Implement:

```text
growth
reproduction
resource consumption
death
decay
competition
predation
resource regeneration
```

Ecology must affect world state.

World state must affect agents.

Agents must affect ecology.

---

# 24. WEATHER

Weather state:

```text
temperature
humidity
pressure
wind
precipitation
visibility
```

Weather affects:

```text
physics
water
vegetation
animals
fly behavior
resource availability
```

Rendering receives consequences.

---

# 25. WATER

Implement a staged approximation:

```text
water volume
surface height
flow
drainage
moisture
```

Later:

```text
erosion
sediment
flooding
```

Do not claim physically exact fluid simulation unless actually implemented and validated.

Use wording:

```text
physically inspired approximation
```

unless a stronger claim is experimentally justified.

---

# 26. FIRE

Implement:

```text
fuel
temperature
moisture
wind influence
spread
burning
extinction
```

Fire must alter:

```text
vegetation
terrain state where implemented
resources
agent danger
ecology
```

---

# 27. OBJECT SYSTEM

Every world object needs a canonical backend state.

At minimum:

```text
ID
position
orientation
mass
material
state
physical properties
interaction capabilities
```

Godot only mirrors this state visually.

---

# 28. CONSTRUCTION SYSTEM

Primitive interactions:

```text
observe
touch
grab
carry
move
rotate
place
connect
disconnect
```

Higher-level construction must emerge from combinations.

Do not create only a scripted:

```text
BUILD_HOUSE
```

function.

---

# 29. LEARNING ENGINE

Implement separately from the Fly-Brain adapter.

Core:

```text
Observation
Action
Outcome
Reward
Prediction
PredictionError
Experience
ReplayBuffer
Learner
Memory
Consolidation
```

---

# 30. EXPERIENCE MODEL

An experience record must include enough information to reproduce its meaning.

Example:

```text
observation
action
world_context
previous_state_reference
outcome
reward
prediction
prediction_error
timestamp
agent_id
policy_version
```

---

# 31. MEMORY

Separate:

```text
working memory
episodic memory
semantic memory
procedural memory
social memory
```

Memory must be persistent.

Memory retrieval must be testable.

---

# 32. LEARNING LOOP

Actual loop:

```text
observe
→ predict
→ act
→ world changes
→ observe outcome
→ calculate error
→ store experience
→ update policy/model
→ consolidate later
```

Test this end to end using a controlled toy environment before connecting the real fly brain.

---

# 33. SAMPLE EFFICIENCY

Measure:

```text
experiences_to_success
retention
generalization
transfer
```

Do not claim human-level learning efficiency without a benchmark.

The benchmark must specify:

```text
task
training budget
environment
seed
model version
metric
result
```

---

# 34. PARALLEL TRAINING

Implement:

```text
EnvironmentWorker
AgentWorker
ExperienceCollector
Learner
```

Architecture:

```text
World workers
→ experience stream
→ learner
→ updated policy/state
→ evaluation
```

Training must run headless.

---

# 35. VULKAN COMPUTE

Build the Vulkan layer separately.

First:

```text
Vulkan instance
device
queue
buffer
descriptor
pipeline
compute dispatch
```

Then a trivial verified kernel.

Do not start with neural simulation.

---

# 36. CPU REFERENCE IMPLEMENTATION

Every important GPU algorithm must have a CPU reference where practical.

Required relationship:

```text
CPU result
vs
GPU result
```

within explicitly documented numerical tolerances.

GPU cannot become the only implementation before verification.

---

# 37. GPU DATA DESIGN

Prefer data-oriented storage:

```text
positions[]
velocities[]
energy[]
states[]
```

instead of excessive pointer-heavy objects.

Document:

```text
alignment
stride
buffer ownership
synchronization
```

---

# 38. GPU KERNEL ORDER

Implement and verify:

```text
vector/math kernel
↓
world-field kernel
↓
sensory kernel
↓
agent batch kernel
↓
learning kernel
↓
neural kernel
↓
population/evolution kernels
```

Every kernel gets:

```text
CPU reference
GPU test
validation output
benchmark
```

---

# 39. MEMORY TRANSFER POLICY

Measure CPU↔GPU transfers.

Do not optimize blindly.

Benchmark:

```text
host→device
device→host
device-only
mapped
staged
```

Record actual results.

---

# 40. EVOLUTION ENGINE

Implement:

```text
Genome
Mutation
Recombination
Reproduction
Selection
Drift
Migration
Population
Species
```

---

# 41. GENOME

Initial genome categories:

```text
body
metabolism
sensory traits
brain development
learning traits
memory traits
social traits
reproductive traits
```

The genome must be serializable and hashable.

---

# 42. MUTATION

Support actual mutation operators:

```text
point
insertion
deletion
duplication
regulatory
structural
```

Mutation distribution must be configurable.

---

# 43. SELECTION

Fitness must be calculated from real world outcomes.

Possible signals:

```text
survival
energy balance
reproduction
offspring survival
resource acquisition
social success
adaptation
```

Do not define fitness simply as:

```text
intelligence_score
```

unless an experiment explicitly uses that artificial objective.

---

# 44. GENETIC DRIFT

Population sampling must permit stochastic drift.

Do not force all changes to be adaptive.

---

# 45. RECOMBINATION

Two-parent reproduction must be supported where biologically appropriate.

Test:

```text
parent genomes
→ recombination
→ offspring
```

with deterministic seeds.

---

# 46. MIGRATION

Agents/populations can move between regions.

Track:

```text
origin
destination
migration event
gene flow
```

---

# 47. SPECIATION

Do not implement:

```text
if distance > X:
    new_species()
```

as the only mechanism.

Implement explicit components for:

```text
population separation
genetic divergence
phenotypic divergence
mating compatibility
offspring viability
```

Then detect species divergence from those states.

---

# 48. BRAIN EVOLUTION

Brain architecture must be allowed to vary through the genome/development system.

Potential traits:

```text
brain volume
cell count
regional allocation
connection density
plasticity
memory capacity
learning architecture
development timing
energy cost
```

No artificial:

```text
brain_size += 1
intelligence += 1
```

shortcut.

---

# 49. BODY/BRAIN CO-EVOLUTION

The body and brain must interact.

Example:

```text
larger brain
→ energy cost
```

and:

```text
better manipulator
→ more valuable planning
```

and:

```text
new body morphology
→ new sensory/motor possibilities
```

---

# 50. GOD FLY

Implement after core learning infrastructure works.

God Fly:

```text
agent
+
memory
+
language
+
LLM
+
teacher role
```

LLM is not the canonical world controller.

---

# 51. GGUF ENGINE

Use a real local GGUF runtime such as llama.cpp after verifying the exact selected model/runtime compatibility.

The model manager must support:

```text
load
unload
health
memory budget
model hash
model version
backend selection
```

---

# 52. MODEL POLICY

Initial roles:

```text
God Fly:
small capable local instruct model

NPC:
smaller economical model

specialist:
optional larger model
```

Do not load unnecessary models permanently.

Measure:

```text
TTFT
tokens/s
RAM
VRAM
context memory
startup time
```

---

# 53. LLM ACTION SECURITY

LLM output must pass:

```text
parse
→ schema validation
→ capability validation
→ world validation
→ action execution
```

Never give raw unrestricted host access to the model.

---

# 54. GOD FLY TEACHING

Teaching outputs:

```text
speech
demonstration
concept
suggestion
explanation
```

Learners must still experience:

```text
observation
practice
feedback
memory
```

Never unlock skills magically because the teacher mentioned them.

---

# 55. NPC SYSTEM

NPCs have:

```text
memory
personality
goals
relationships
knowledge
history
```

LLM inference is event-driven.

Do not run an LLM continuously for every background agent.

---

# 56. LANGUAGE

Implement communication as a separate system.

Start with:

```text
signals
symbols
meaning associations
```

Then optionally:

```text
vocabulary
sequence patterns
grammar
```

Store language state.

Do not claim an emergent language exists until actual interactions demonstrate it.

---

# 57. SOCIAL LEARNING

Implement:

```text
observation
imitation
demonstration
teaching
communication
```

Track whether knowledge originated from:

```text
individual experience
social transfer
God Fly
cultural inheritance
```

---

# 58. TECHNOLOGY ENGINE

Primitive capabilities:

```text
inspect
manipulate
combine
connect
operate
construct
```

Later:

```text
machines
energy
computation
programming
automation
```

Everything must remain sandboxed.

---

# 59. PROGRAMMABLE WORLD

Implement a safe virtual execution layer.

The evolved agents may interact with:

```text
virtual programs
virtual machines
virtual devices
virtual networks
```

but never receive arbitrary host-level execution privileges.

---

# 60. HEADLESS EXECUTABLE

The backend must produce something equivalent to:

```text
flgod --headless
```

It must run without:

```text
Godot editor
display
renderer
camera
UI
```

---

# 61. HEADLESS MODES

Provide:

```text
--self-test
--benchmark
--simulate
--train
--evolve
--validate
--checkpoint
--restore
--replay
```

Each mode must have real implementation and tests.

---

# 62. PERSISTENCE

Checkpoint:

```text
world
agents
brains
memory
genomes
language
culture
technology
RNG
simulation tick
generation
software version
model versions
```

---

# 63. CHECKPOINT HASHING

Compute:

```text
world_hash
agent_hash
brain_hash
genome_hash
memory_hash
language_hash
state_hash
```

Use stable serialization before hashing.

---

# 64. REPLAY

Replay must reproduce the specified simulation state within the project's deterministic contract.

Record:

```text
checkpoint
seed
event log
versions
configuration
```

---

# 65. CRASH RECOVERY

Test:

```text
run
→ checkpoint
→ terminate
→ restore
→ continue
```

Compare state to an uninterrupted reference run.

---

# 66. EXPERIMENT MANIFEST

Each experiment receives:

```text
experiment_id
timestamp
source_commit
simulation_version
world_seed
agent_seed
genome_seed
model_versions
configuration
checkpoint
result
hashes
```

No undocumented experiments.

---

# 67. BACKEND QUALITY GATE

The backend is not considered complete until:

```text
[ ] source builds cleanly
[ ] unit tests pass
[ ] integration tests pass
[ ] deterministic tests pass
[ ] CPU execution works
[ ] Vulkan execution works
[ ] CPU/GPU comparison passes
[ ] physics tests pass
[ ] world generation tests pass
[ ] learning tests pass
[ ] evolution tests pass
[ ] persistence tests pass
[ ] replay tests pass
[ ] recovery tests pass
[ ] headless execution works
```

Store actual evidence.

---

# 68. ONLY THEN FRONTEND

Do not start the final visual frontend before the backend gate is green.

---

# 69. GODOT FRONTEND

Use Godot as presentation/application layer.

The backend remains canonical.

Godot receives:

```text
world state
agent state
events
camera targets
telemetry
```

Godot renders:

```text
terrain
vegetation
water
weather
animals
flies
objects
structures
lighting
particles
UI
```

---

# 70. GODOT RENDERER

Target:

```text
Godot 4
Forward+
Vulkan
```

Verify the exact stable version actually installed.

Do not hard-code a version based on memory.

Query the official release/source first.

---

# 71. TERRAIN

If Terrain3D is used:

1. inspect current upstream compatibility
2. determine exact Godot compatibility
3. build against the selected version
4. test
5. only then integrate

If compatibility is not verified:

```text
do not claim compatibility
```

Fallback:

```text
custom terrain renderer
```

if required.

---

# 72. TERRAIN RENDERING

Implement:

```text
LOD
streaming
instancing
terrain material
biome materials
vegetation distribution
```

Use backend terrain data as source of truth.

---

# 73. VEGETATION

Do not instantiate every blade/tree as an independent heavy scene node.

Use GPU-friendly instancing where appropriate.

Backend controls:

```text
species
position
growth
health
state
```

Godot renders it.

---

# 74. WATER RENDERING

Backend:

```text
height
flow
state
```

Godot:

```text
surface
reflection
refraction
foam
visual ripples
```

---

# 75. WEATHER RENDERING

Backend:

```text
rain intensity
wind
visibility
humidity
temperature
```

Godot:

```text
clouds
rain
fog
wet surfaces
lighting
```

---

# 76. LIGHTING

Create quality profiles:

```text
LOW
MEDIUM
HIGH
CINEMATIC
```

Every expensive visual feature must be optional.

---

# 77. PHYSICS VISUAL SYNC

Canonical transform comes from simulation.

Godot mirrors it.

Godot must not silently introduce a different position.

---

# 78. FLY VISUALIZATION

The fly model is only visual.

Its actual body state originates in simulation.

Animations are driven from:

```text
velocity
orientation
behavior
wing state
```

---

# 79. CINEMATIC CAMERA ENGINE

Build a dedicated system:

```text
CameraDirector
TargetSelector
ShotPlanner
FocusController
CollisionAvoidance
MovementController
TransitionController
```

---

# 80. CAMERA INPUTS

The director receives:

```text
agent position
velocity
importance
novelty
event severity
environmental beauty
causal relevance
camera history
```

---

# 81. SHOT TYPES

Implement actual reusable shot types:

```text
macro
close
medium
wide
establishing
tracking
orbit
overhead
low angle
POV
reaction shot
```

---

# 82. CAMERA QUALITY TESTS

Test:

```text
subject remains framed
camera avoids collision
camera does not teleport
focus transitions correctly
camera follows predicted movement
shot remains within configured limits
```

---

# 83. FOUR-CAMERA SYSTEM

Four independent camera directors:

```text
camera 1
God Fly

camera 2
learning agent

camera 3
event

camera 4
environment / alternate colony
```

All can observe the same simulation independently.

---

# 84. FLGODTV UI

Implement:

```text
LIVE
GENERATION
DAY
COLONY
AGENT
EVENT
```

Optional research panels:

```text
BRAIN
MEMORY
LANGUAGE
EVOLUTION
GENOME
TECHNOLOGY
```

Do not show fake metrics.

---

# 85. TELEMETRY

Every displayed metric must come from real simulation data.

If unavailable:

```text
N/A
```

not:

```text
73%
```

invented for visual appearance.

---

# 85B. FRONTEND RUNTIME RENDERING PIPELINE PLAN

Based on repository diagnosis (Root cause of grey screen: camera frustum pointing into unpopulated void, single-viewport limitation disabling 3 cameras, 62x62 terrain undersized for colony positions at (60,60), and hardcoded nonexistent IPC paths), the frontend is engineered and verified in 16 strict sequential phases:

1. **Runtime / Environment Validation:** Detect Godot 4.7.2 Forward+ Vulkan runtime, verify physical GPU device (AMD Radeon RDNA2), and validate windowed vs headless operation.
2. **Minimal Godot Rendering Smoke Test:** Verify smallest possible 3D rendering pipeline (WorldEnvironment, DirectionalLight3D, Camera3D, MeshInstance3D) producing objectively verifiable non-grey pixel output.
3. **Deterministic Fallback Camera:** Establish known-good fallback camera (known position, known target, known FOV, valid near/far clipping, current=true) that guarantees the scene is always framed and visible, even if backend camera data is absent or malformed.
4. **WorldEnvironment & Lighting:** Procedural sky, sun directional light, ambient fill, and tone mapping ensuring that background space never renders as an unlit grey void.
5. **Terrain Rendering:** Expand procedural terrain mesh coverage to fully encompass all colony nests (192x192 extent) with Whittaker biome coloration and dynamic LOD streaming.
6. **Vegetation, Water & Weather Rendering:** Scale water plane (240x240) and vegetation distribution to match world bounds; enable GPU rain particles and fog cues.
7. **Agent MultiMesh Rendering:** Render agent flies across all colonies using Blender 3D procedural meshes or fallback spheres with colony color differentiation.
8. **God Fly Visual Mesh:** Render the God Fly with golden emissive material and distinct transform synchronization.
9. **BackendBridge Integration:** Implement canonical IPC snapshot reader with multi-path resolution (app directory, working directory, CLI argument) and strict "Disconnected / N/A" reporting when offline.
10. **Canonical State Visualization:** Update world, agent, and weather visual state dynamically upon receipt of real backend simulation frames.
11. **Telemetry Stream:** Extract real simulation clock, tick rate, connectome soma rate, and memory records without mock numbers.
12. **Broadcast HUD / UI:** Comprehensive FLGODTV interface with LIVE status, 4-camera badges, population counters, weather panel, and collapsible research metrics.
13. **Four-Camera Presentation System:** Multi-viewport quad-view presentation (or full-screen toggleable view modes: QuadView, GodFly, LearningAgent, Colony, Event) with independent camera tracking.
14. **User Input & Interaction:** Keyboard controls (Keys 1-5 for camera modes, R for research toggle, H for HUD toggle, Space for pause).
15. **Performance & Frame Stability:** Verify smooth 60 FPS operation with bounded GPU memory and zero unhandled script errors.
16. **Complete Local Verification:** Automated visual screenshot capture, CTest test suite pass (52/52), and persistent evidence logging.

---

# 86. VIDEO ENGINE

Video generation is separate from normal gameplay.

Pipeline:

```text
simulation
→ event archive
→ event ranking
→ shot planning
→ multi-angle rendering
→ take selection
→ timeline
→ encode
```

---

# 87. EVENT DETECTOR

Search for genuine state transitions:

```text
new behavior
new capability
first tool use
first construction
new communication pattern
brain architectural change
morphological divergence
population split
new technology
major failure
extinction
speciation
```

---

# 88. EVENT EVIDENCE

Every highlighted event must reference:

```text
simulation tick
agent
generation
experiment
checkpoint
state hash
```

Then the video is traceable to the simulation.

---

# 89. BEST-OF SCORING

Possible real signals:

```text
novelty
rarity
causal importance
evolutionary importance
visual quality
behavioral importance
```

Do not claim an event is significant merely because the scorer says so.

The scoring system is a ranking heuristic.

---

# 90. AUTOMATIC VIDEO

Generate:

```text
event clips
episode timeline
metadata
render manifest
final video
```

FFmpeg may be used for encoding/muxing.

Godot can be used for the actual visual render.

---

# 91. VIDEO EVIDENCE

Store:

```text
render_manifest.json
event_manifest.json
camera_plan.json
frame_count
video checksum
source experiment
```

---

# 92. BACKEND + FRONTEND INTEGRATION

Use a versioned protocol.

Example concepts:

```text
WorldSnapshot
AgentSnapshot
EventMessage
CameraTarget
TelemetryMessage
```

Every message requires:

```text
protocol_version
simulation_version
tick
```

---

# 93. INTEGRATION RULE

Test:

```text
headless backend
→ world
→ agent
→ event
→ frontend
→ rendered state
```

Never make visual output the only integration test.

---

# 94. LOCAL BUILD MATRIX

Windows:

```text
Debug
Release
Headless
```

Linux:

```text
Debug
Release
Headless
```

For each:

```text
configure
build
unit tests
integration tests
self test
sample execution
artifact generation
```

---

# 95. CROSS-PLATFORM POLICY

Avoid:

```text
hard-coded Windows paths
hard-coded drive letters
Windows-only filesystem assumptions
```

Use:

```text
CMake
std::filesystem
environment/configuration
platform abstraction
```

---

# 96. WINDOWS TEST

Run locally.

Capture:

```text
compiler
OS
GPU
Vulkan
build
tests
runtime
```

Store in:

```text
evidence/windows/
```

---

# 97. LINUX TEST

Build on a real Linux environment or real Linux CI runner.

Do not claim Linux support because the code appears portable.

Linux must actually build and test.

Store:

```text
evidence/linux/
```

---

# 98. GODOT CI

Run:

```text
headless
project validation
script/static checks
test scene
```

Do not depend on interactive editor actions.

---

# 99. CLOUD CI

Create CI only AFTER local builds work.

Do not use CI as a substitute for local debugging.

---

# 100. GITHUB REPOSITORY CREATION

After the local project reaches the release gate:

1. create a new GitHub repository
2. initialize remote
3. push source
4. push workflows
5. push documentation
6. push reproducibility manifests
7. keep huge binary assets/models outside Git unless explicitly intended
8. use release artifacts where appropriate

Never push secrets.

Never push API tokens.

Never push credentials.

Never push unnecessary private data.

---

# 101. GITHUB ACTIONS

Create workflows for:

```text
windows-build.yml
linux-build.yml
tests.yml
determinism.yml
vulkan-check.yml
godot-check.yml
release.yml
```

Only create workflows that can actually execute in the chosen runner environment.

---

# 102. CI MATRIX

At minimum:

```text
Windows
Linux
```

and configurations:

```text
Release
Headless
Tests
```

GPU-dependent jobs may require dedicated self-hosted runners or explicitly supported environments.

Never fake GPU validation on CPU and call it Vulkan validation.

---

# 103. GPU CI POLICY

If a cloud runner has no compatible GPU:

```text
VULKAN GPU TEST = NOT AVAILABLE
```

Do not call it:

```text
PASS
```

Use self-hosted runners when real GPU verification is required.

---

# 104. CI ARTIFACTS

Every significant CI run should retain:

```text
build logs
test logs
binary artifacts
test reports
manifests
hashes
```

where practical.

---

# 105. CLOUD MONITORING LOOP

After pushing:

```text
push
→ inspect workflow
→ inspect every job
→ inspect failures
→ fetch logs
→ reproduce locally
→ fix
→ commit
→ push
→ rerun
```

Do this repeatedly until all required jobs are actually green.

---

# 106. NO GREEN-LIGHT BY ASSUMPTION

Do not stop because:

```text
workflow started
```

Do not stop because:

```text
most jobs passed
```

Do not stop because:

```text
build appears successful
```

Stop only when the defined release matrix is actually green or an explicitly documented external limitation remains.

---

# 107. RELEASE CANDIDATE

Create:

```text
RC
```

only after:

```text
local Windows
local Linux
CI Windows
CI Linux
backend
frontend
integration
determinism
```

have passed.

---

# 108. FINAL SELF-TEST

A full end-to-end test must execute:

```text
create seed
↓
generate world
↓
spawn agent
↓
run simulation
↓
physics
↓
learning
↓
checkpoint
↓
restore
↓
continue
↓
generate event
↓
camera selection
↓
Godot render
↓
video output
↓
hash output
```

Store the result.

---

# 109. REPRODUCIBILITY

The same tagged release must document:

```text
source commit
dependencies
versions
models
model checksums
configuration
seed
platform
```

The experiment must be repeatable.

---

# 110. PERFORMANCE

Do not optimize before measuring.

For every major optimization:

```text
before benchmark
after benchmark
correctness comparison
memory comparison
```

Record real measurements.

Never invent:

```text
10× faster
50% less VRAM
```

without executed measurements.

---

# 111. PROFILING

Use appropriate platform profilers.

Measure:

```text
CPU
GPU
memory
GPU memory
PCIe transfers
LLM inference
simulation tick
physics
rendering
```

---

# 112. ERROR HANDLING

Errors must contain:

```text
subsystem
operation
entity if relevant
tick
version
useful context
```

Never:

```text
something went wrong
```

when meaningful diagnostics are available.

---

# 113. LOGGING

Levels:

```text
TRACE
DEBUG
INFO
WARN
ERROR
FATAL
```

Headless runs must have machine-readable logs where practical.

---

# 114. DEBUGGABILITY

Every major subsystem needs:

```text
startup diagnostics
health check
self test
configuration dump
version dump
```

The project should be easy to debug without launching Godot.

---

# 115. CONFIGURATION

Centralize runtime configuration.

Example concepts:

```text
simulation_config
world_config
physics_config
learning_config
evolution_config
llm_config
render_config
```

Never scatter magic numbers across the codebase.

---

# 116. VERSIONING

Every persisted state should know:

```text
simulation_version
schema_version
brain_version
world_generator_version
model_version
```

---

# 117. SCHEMA MIGRATIONS

When state formats change:

```text
old schema
→ migration
→ new schema
```

Never silently reinterpret incompatible binary data.

---

# 118. LICENSES

For every dependency:

```text
license
source
notice requirements
```

Create appropriate notices.

Never assume a GitHub repository is automatically license-free.

---

# 119. SECURITY

Do not commit:

```text
tokens
keys
passwords
cookies
private endpoints
```

Use environment variables/secrets where necessary.

Never allow LLM output to gain arbitrary host command execution.

---

# 120. DOCUMENTATION RULE

Documentation must describe reality.

Allowed:

```text
implemented
tested
verified
experimental
partially implemented
not yet implemented
```

Forbidden:

```text
revolutionary
perfect
human-level
production-ready
physically exact
fully autonomous
unlimited
```

unless those claims have a precise evidence-backed definition.

---

# 121. README RULE

README must be rewritten LAST.

Before writing any claim:

```text
locate evidence
verify evidence
check current commit
write only supported statement
```

---

# 122. README STRUCTURE

Use:

```text
Project
Status
Architecture
Requirements
Installation
Build
Headless usage
Godot usage
Models
Experiments
Testing
Reproducibility
Known limitations
Evidence
License
```

---

# 123. EVIDENCE SECTION

README may link to:

```text
evidence/
CI runs
benchmark reports
determinism reports
experiment manifests
screenshots
videos
release artifacts
```

Only include evidence that actually exists.

---

# 124. STATUS BADGES

A badge must correspond to a real external state.

Never add:

```text
Build Passing
```

before CI actually passes.

Never add:

```text
Linux Supported
```

before Linux is actually tested.

Never add:

```text
Vulkan Supported
```

before real Vulkan verification.

---

# 125. NO MARKETING README

The README is a technical document.

Write:

```text
At commit X, test Y passed on platform Z.
```

rather than:

```text
The world's most advanced evolutionary AI simulation.
```

---

# 126. FINAL RELEASE MANIFEST

Generate:

```text
release_manifest.json
```

containing:

```text
release version
source commit
dependency versions
model versions
model checksums
platforms tested
test suite result
build artifacts
artifact hashes
known limitations
```

---

# 127. FINAL RELEASE CHECK

Must confirm:

```text
[ ] source clean
[ ] no secrets
[ ] no accidental binaries
[ ] dependencies documented
[ ] licenses documented
[ ] Windows build passed
[ ] Linux build passed
[ ] backend tests passed
[ ] frontend tests passed
[ ] integration tests passed
[ ] deterministic test passed
[ ] checkpoint test passed
[ ] recovery test passed
[ ] Vulkan test passed where GPU hardware exists
[ ] Godot test passed
[ ] LLM load test passed
[ ] video pipeline test passed
[ ] release artifact verified
[ ] release manifest generated
[ ] README claims verified
```

---

# 128. AUTONOMOUS REPAIR LOOP

For every failure:

```text
FAILURE
↓
capture exact evidence
↓
classify:
build / dependency / code / test / runtime / platform / GPU
↓
find root cause
↓
make smallest correct fix
↓
rebuild
↓
rerun affected tests
↓
rerun broader regression suite
↓
verify
↓
commit
```

Never patch symptoms while ignoring root cause.

---

# 129. REGRESSION RULE

After any change affecting:

```text
core
world
physics
GPU
learning
evolution
LLM
protocol
persistence
Godot bridge
```

run the relevant regression suites.

For major changes run the full suite.

---

# 130. GIT POLICY

Commits should be small and meaningful.

Examples:

```text
core: add deterministic simulation clock
world: add chunk seed generation
physics: add jolt bridge
gpu: add validated compute backend
learning: add replay buffer
evolution: add mutation operators
llm: add local model gateway
godot: add world-state bridge
camera: add event-directed shot planner
video: add best-of renderer
ci: add linux release workflow
docs: document verified capabilities
```

Do not make one giant opaque commit containing unrelated changes.

---

# 131. BRANCH POLICY

Use isolated branches for large features.

Before merge:

```text
build
test
inspect diff
verify evidence
```

---

# 132. NO DEAD CODE

Do not leave obsolete duplicate systems.

After replacement:

```text
remove old path
update tests
update docs
```

unless intentionally preserved for compatibility.

---

# 133. CLEAN BUILD TEST

At major milestones:

```text
delete build directory
reconfigure
rebuild from source
run tests
```

This catches accidental dependency on stale artifacts.

---

# 134. CLEAN MACHINE TEST

Before release, simulate a clean environment as closely as practical:

```text
fresh build directory
fresh dependency resolution
fresh configuration
fresh model verification
```

Document anything requiring preinstalled external components.

---

# 135. MODEL REPRODUCIBILITY

For every GGUF:

```text
repository
filename
quantization
size
checksum
download source
runtime version
```

Do not rely on a mutable tag alone.

---

# 136. LLM BENCHMARK

For every selected model:

```text
load time
TTFT
tokens/s
memory
context size
Vulkan usage if applicable
CPU usage
```

Save benchmark output.

---

# 137. MODEL SELECTION RULE

Never say:

```text
best model
```

without defining:

```text
best for what?
```

Use measured criteria:

```text
quality
speed
memory
stability
tool use
dialogue
teaching
```

---

# 138. LEARNING BENCHMARK

Create reproducible tasks:

```text
navigation
memory
object interaction
social learning
tool use
construction
communication
generalization
```

Record:

```text
training budget
success rate
sample count
retention
```

---

# 139. EVOLUTION BENCHMARK

Record:

```text
population
generations
mutation settings
selection settings
diversity
speciation events
morphological divergence
brain divergence
```

---

# 140. PHYSICS BENCHMARK

Record:

```text
step time
active bodies
collision count
constraint count
stability
```

Do not call the simulation „real-world accurate“ unless validated against a defined reference.

Preferred language:

```text
physically based
physics-informed
simulated
approximate
```

where appropriate.

---

# 141. WORLD BENCHMARK

Record:

```text
chunk generation time
memory
terrain generation
vegetation generation
weather update
ecology update
streaming latency
```

---

# 142. RENDER BENCHMARK

Record:

```text
resolution
FPS
GPU memory
draw calls
frame time
terrain load
vegetation count
camera count
```

---

# 143. VIDEO BENCHMARK

Record:

```text
render time
FPS
resolution
frame count
encoding time
file size
checksum
```

---

# 144. FINAL PROJECT TEST LOOP

At the end of development, repeatedly execute:

```text
LOCAL CLEAN BUILD
↓
UNIT TESTS
↓
INTEGRATION TESTS
↓
HEADLESS TESTS
↓
GPU TESTS
↓
GODOT TESTS
↓
VIDEO TESTS
↓
WINDOWS TEST
↓
LINUX TEST
↓
PACKAGE
↓
GITHUB PUSH
↓
CI
↓
READ LOGS
↓
FIX
↓
REPEAT
```

This loop continues until the release gate is actually green.

---

# 145. IMPORTANT: DO NOT STOP AFTER ONE SUCCESSFUL RUN

A successful run proves only that run.

For critical functionality use repeated validation where useful:

```text
different seeds
different configurations
repeated checkpoint restore
multiple worlds
multiple agents
multiple generations
```

---

# 146. CHAOS / FAILURE TESTING

Where practical, deliberately test:

```text
invalid config
missing model
corrupt checkpoint
bad world chunk
GPU unavailable
network unavailable
LLM unavailable
render unavailable
unexpected process termination
```

The project should fail loudly and recover safely where designed.

---

# 147. FALLBACK POLICY

CPU fallback:

```text
GPU unavailable
→ CPU reference
```

LLM unavailable:

```text
God Fly degraded mode
```

Renderer unavailable:

```text
headless simulation remains functional
```

Network unavailable:

```text
local simulation remains functional
```

Do not confuse fallback with full feature parity.

Document degraded modes.

---

# 148. FINAL ARCHITECTURE

```text
                    FLGODTV
                       │
             ┌─────────┴─────────┐
             │                   │
       HEADLESS SIM           GODOT
             │                   │
        C++ CORE             Vulkan
             │                   │
    ┌────────┼────────┐      rendering
    │        │        │         │
  WORLD    PHYSICS   AI       CAMERA
    │        │        │         │
    │      JOLT       │        VIDEO
    │                 │
    └────────┬────────┘
             │
          VULKAN
          COMPUTE
             │
    ┌────────┼───────────┐
    │        │           │
  NEURAL   LEARNING    ECOLOGY
    │        │           │
    └────────┼───────────┘
             │
          EVOLUTION
             │
         POPULATION
             │
      ┌──────┴───────┐
      │              │
  NORMAL FLIES    GOD FLY
      │              │
  Fly-Brain        GGUF
      │              │
      └──────┬───────┘
             │
          LANGUAGE
             │
          CULTURE
             │
        TECHNOLOGY
             │
      WORLD MODIFICATION
             │
        NEW SELECTION
             │
        NEW SPECIES
```

---

# 149. ABSOLUTE RULE

The agent must always prefer:

```text
PROVE
```

over:

```text
ASSUME
```

and:

```text
MEASURE
```

over:

```text
GUESS
```

and:

```text
FIX
```

over:

```text
SKIP
```

and:

```text
DOCUMENT LIMITATIONS
```

over:

```text
HIDE LIMITATIONS
```

---

# 150. FINAL COMPLETION CRITERION

The project is finished only when:

```text
the backend really runs
AND
the frontend really runs
AND
the two communicate
AND
the simulation can run headless
AND
the GPU path is verified where hardware exists
AND
the physics path is verified
AND
learning actually changes behavior
AND
evolution actually changes populations
AND
checkpoint/restore works
AND
Windows builds
AND
Linux builds
AND
CI passes
AND
the video pipeline actually renders
AND
the final README contains only verified claims
AND
the release artifacts are reproducible
```

Not:

```text
"the code looks complete"
```

Not:

```text
"tests were skipped because environment..."
```

Not:

```text
"should work"
```

The final answer of every major stage must distinguish:

```text
VERIFIED
PARTIAL
UNVERIFIED
BLOCKED
```

and never present an unverified feature as complete.

---

# 151. REQUIRED FINAL REPORT

At completion produce:

```text
docs/FINAL_VERIFICATION.md
```

containing:

```text
project commit
build versions
dependency versions
models
model hashes
platforms
test commands
test results
benchmark results
known limitations
artifacts
CI run references
release hashes
```

Every major claim must point to actual evidence.

---

# 152. AUTONOMOUS AGENT BEHAVIOR

Work autonomously.

Do not repeatedly ask for confirmation for ordinary engineering decisions.

When an implementation decision is ambiguous:

1. inspect existing architecture
2. inspect upstream documentation
3. choose the most reproducible solution
4. document the decision
5. implement
6. test
7. revise if evidence contradicts the decision

Do not stop merely because a task is large.

Proceed incrementally through verified stages.

---

# 153. STAGE GATES

The agent must maintain:

```text
STAGE 0 — AUDIT
STAGE 1 — TOOLCHAIN
STAGE 2 — CORE
STAGE 3 — WORLD
STAGE 4 — PHYSICS
STAGE 5 — HEADLESS BACKEND
STAGE 6 — VULKAN
STAGE 7 — LEARNING
STAGE 8 — EVOLUTION
STAGE 9 — AGENTS
STAGE 10 — LLM
STAGE 11 — LANGUAGE
STAGE 12 — TECHNOLOGY
STAGE 13 — FLY-BRAIN ADAPTER
STAGE 14 — MULTI-AGENT
STAGE 15 — BACKEND FINAL
STAGE 16 — GODOT
STAGE 17 — WORLD RENDERING
STAGE 18 — CAMERA
STAGE 19 — UI
STAGE 20 — VIDEO
STAGE 21 — INTEGRATION
STAGE 22 — WINDOWS
STAGE 23 — LINUX
STAGE 24 — LOCAL RELEASE
STAGE 25 — GITHUB
STAGE 26 — CLOUD CI
STAGE 27 — RELEASE VERIFICATION
STAGE 28 — README
STAGE 29 — FINAL AUDIT
```

Never mark a stage complete without its gate.

---

# 154. STAGE STATE FORMAT

Maintain:

```text
docs/STAGE_STATUS.md
```

Example:

```text
STAGE 00
STATUS: VERIFIED

Evidence:
- audit report
- repository inventory
- command logs

STAGE 01
STATUS: VERIFIED

Evidence:
- toolchain report
- Vulkan report

STAGE 02
STATUS: IN PROGRESS
```

Never report:

```text
DONE
```

when evidence is absent.

---

# 155. FINAL PHILOSOPHY

The system should eventually allow:

```text
realistic physical interaction
+
continuous procedural worlds
+
learning
+
social communication
+
technology
+
evolution
```

but the engineering process itself must remain boringly rigorous:

```text
inspect
build
test
measure
verify
document
repeat
```

The simulation may become strange.

The codebase must not.

# END OF CONTRACT