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

func ensure_initialized() -> void:
	if not bridge:
		bridge = get_node_or_null("BackendBridge")
	if not camera_global:
		camera_global = get_node_or_null("CameraRig/CameraGlobal")
		camera_godfly = get_node_or_null("CameraRig/CameraGodFly")
		camera_colony = get_node_or_null("CameraRig/CameraColony")
		camera_event = get_node_or_null("CameraRig/CameraEvent")
	if not fly_multimesh:
		fly_multimesh = get_node_or_null("FlyMultiMesh")
		god_fly_mesh = get_node_or_null("GodFlyMesh")

func _ready() -> void:
	print("[FLGODTVController] Initializing 4-Camera System and MultiMesh Fly Renderer...")
	ensure_initialized()
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
	if not bridge:
		return

	# If backend bridge supplies authoritative camera channel data (GEMINI.md Sections 79, 83)
	if bridge.camera_data.has("channels") and bridge.camera_data["channels"] is Array:
		var channels: Array = bridge.camera_data["channels"]
		for ch in channels:
			if not (ch is Dictionary) or not ch.has("channel"):
				continue
			var ch_id: int = ch["channel"]
			var pose: Dictionary = ch.get("current_pose", {})
			if pose.is_empty():
				continue
			var pos_arr: Array = pose.get("pos", [0, 10, 20])
			var look_arr: Array = pose.get("look_at", [0, 0, 0])
			var fov_val: float = pose.get("fov", 60.0)

			var target_pos := Vector3(pos_arr[0], pos_arr[1], pos_arr[2])
			var target_look := Vector3(look_arr[0], look_arr[1], look_arr[2])

			match ch_id:
				0: # Cam1_GodFly
					if camera_godfly:
						var new_pos = camera_godfly.position.lerp(target_pos, delta * 5.0)
						camera_godfly.look_at_from_position(new_pos, target_look, Vector3.UP)
						camera_godfly.fov = fov_val
				1: # Cam2_LearningAgent
					pass
				2: # Cam3_Event
					if camera_event:
						var new_pos = camera_event.position.lerp(target_pos, delta * 4.0)
						camera_event.look_at_from_position(new_pos, target_look, Vector3.UP)
						camera_event.fov = fov_val
				3: # Cam4_EnvironmentColony
					if camera_global:
						camera_global.look_at_from_position(target_pos, target_look, Vector3.UP)
						camera_global.fov = fov_val
		return

	# Fallback camera positioning if backend camera stream is absent
	if camera_global:
		camera_global.look_at_from_position(Vector3(30, 45, 85), Vector3(30, 0, 30), Vector3.UP)

	if camera_godfly and bridge.god_fly_data.has("position"):
		var gf_pos: Vector3 = bridge.god_fly_data["position"]
		var target_pos := gf_pos + Vector3(0, 3.5, 6.0)
		var new_pos = camera_godfly.position.lerp(target_pos, delta * 4.0)
		camera_godfly.look_at_from_position(new_pos, gf_pos, Vector3.UP)

	if camera_colony and bridge.colonies_data.size() > 0:
		var nest_pos: Vector3 = bridge.colonies_data[0].get("nest", Vector3.ZERO)
		camera_colony.look_at_from_position(nest_pos + Vector3(10, 8, 12), nest_pos, Vector3.UP)

	if camera_event and bridge.agents_data.size() > 0:
		var agent_pos: Vector3 = bridge.agents_data[0].get("position", Vector3.ZERO)
		var new_pos = camera_event.position.lerp(agent_pos + Vector3(2.0, 1.5, 3.0), delta * 2.5)
		camera_event.look_at_from_position(new_pos, agent_pos, Vector3.UP)

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
