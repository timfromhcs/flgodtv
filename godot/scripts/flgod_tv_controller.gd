extends Node3D
class_name FLGODTVController

# FLGODTVController coordinates the visual presentation, 4-camera system,
# and rendering of backend canonical state in Godot 4 Forward+ Vulkan.

@onready var bridge: Node = $BackendBridge
@onready var camera_global: Camera3D = $CameraRig/CameraGlobal
@onready var camera_godfly: Camera3D = $CameraRig/CameraGodFly
@onready var camera_colony: Camera3D = $CameraRig/CameraColony
@onready var camera_event: Camera3D = $CameraRig/CameraEvent

@onready var fly_multimesh: MultiMeshInstance3D = $FlyMultiMesh
@onready var god_fly_mesh: MeshInstance3D = $GodFlyMesh

# Active camera presentation mode: 0=QuadView (4-cam), 1=Global, 2=GodFly, 3=Colony, 4=Cinematic
var current_view_mode: int = 0

func _ready() -> void:
	print("[FLGODTVController] Initializing 4-Camera System and MultiMesh Fly Renderer...")
	if bridge:
		bridge.state_updated.connect(_on_state_updated)
	setup_fly_multimesh()

func setup_fly_multimesh() -> void:
	if not fly_multimesh:
		return
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.instance_count = 100
	
	# Create simple proxy prism/capsule mesh for flies
	var sphere := SphereMesh.new()
	sphere.radius = 0.2
	sphere.height = 0.5
	mm.mesh = sphere
	fly_multimesh.multimesh = mm

func _process(delta: float) -> void:
	update_cameras(delta)
	update_fly_instances()

func _on_state_updated(_state: Dictionary) -> void:
	# Receives continuous state updates from backend bridge
	pass

func update_cameras(delta: float) -> void:
	# 1. Global Camera: smooth orbiting over world center
	if camera_global:
		camera_global.position = Vector3(30, 45, 85)
		camera_global.look_at(Vector3(30, 0, 30), Vector3.UP)

	# 2. God Fly Camera: follows God Fly position with cinematic offset
	if camera_godfly and bridge and bridge.god_fly_data.has("position"):
		var gf_pos: Vector3 = bridge.god_fly_data["position"]
		var target_pos := gf_pos + Vector3(0, 3.5, 6.0)
		camera_godfly.position = camera_godfly.position.lerp(target_pos, delta * 4.0)
		camera_godfly.look_at(gf_pos, Vector3.UP)

	# 3. Colony Camera: focused on active colony nest
	if camera_colony and bridge and bridge.colonies_data.size() > 0:
		var nest_pos: Vector3 = bridge.colonies_data[0].get("nest", Vector3.ZERO)
		camera_colony.position = nest_pos + Vector3(10, 8, 12)
		camera_colony.look_at(nest_pos, Vector3.UP)

	# 4. Cinematic Event Camera: dynamic tracking of active agents
	if camera_event and bridge and bridge.agents_data.size() > 0:
		var agent_pos: Vector3 = bridge.agents_data[0].get("position", Vector3.ZERO)
		camera_event.position = camera_event.position.lerp(agent_pos + Vector3(2.0, 1.5, 3.0), delta * 2.5)
		camera_event.look_at(agent_pos, Vector3.UP)

func update_fly_instances() -> void:
	if not fly_multimesh or not fly_multimesh.multimesh or not bridge:
		return
	var mm := fly_multimesh.multimesh
	var agents: Array = bridge.agents_data
	var count : int = min(agents.size(), mm.instance_count)
	for i in range(count):
		var a: Dictionary = agents[i]
		var pos: Vector3 = a.get("position", Vector3.ZERO)
		var t := Transform3D()
		t.origin = pos
		mm.set_instance_transform(i, t)
		var col := Color(0.2, 0.8, 1.0) if a.get("colony_id", 1) == 1 else Color(1.0, 0.6, 0.2)
		mm.set_instance_color(i, col)

	# Update God Fly visual mesh
	if god_fly_mesh and bridge.god_fly_data.has("position"):
		god_fly_mesh.position = bridge.god_fly_data["position"]
