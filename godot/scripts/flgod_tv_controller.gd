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

# View mode: 0=QuadView (4-cam presentation), 1=GodFly Solo, 2=Colony Solo, 3=Event Solo, 4=Global Wide
var current_view_mode: int = 4

# QuadView Presentation overlay container and sub-viewports
var quad_container: Control = null
var quad_sub_cams: Array[Camera3D] = []

# Visual V2 Subsystems: MapSystem & AtmosphereController
const MapSystemScript = preload("res://scripts/map_system.gd")
const AtmosphereControllerScript = preload("res://scripts/atmosphere_controller.gd")
var map_system: Control = null
var atmosphere_ctrl: Node = null
var is_auto_broadcast: bool = false
var broadcast_hold_timer: float = 0.0

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
	if fly_multimesh and fly_multimesh.multimesh == null:
		setup_fly_multimesh()
	if quad_container == null:
		setup_quad_view_presentation()
	if map_system == null:
		setup_map_system()
	if atmosphere_ctrl == null:
		setup_atmosphere_controller()

func _ready() -> void:
	print("[FLGODTVController] Initializing 4-Camera System and MultiMesh Fly Renderer...")
	ensure_initialized()
	if bridge:
		bridge.state_updated.connect(_on_state_updated)
	set_view_mode(current_view_mode)

func setup_map_system() -> void:
	var ui_node = get_node_or_null("UI")
	if not ui_node or map_system != null:
		return
	map_system = MapSystemScript.new()
	map_system.name = "MinimapRadar"
	map_system.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT)
	map_system.position = Vector2(-200.0, 20.0)
	ui_node.add_child(map_system)
	if bridge:
		map_system.set_bridge(bridge)

func setup_atmosphere_controller() -> void:
	if atmosphere_ctrl != null:
		return
	atmosphere_ctrl = AtmosphereControllerScript.new()
	atmosphere_ctrl.name = "AtmosphereController"
	add_child(atmosphere_ctrl)

func load_mesh_from_scene(path: String, fallback_mesh: Mesh) -> Mesh:
	if ResourceLoader.exists(path):
		var res = load(path)
		if res is PackedScene:
			var inst = res.instantiate()
			if inst is MeshInstance3D and inst.mesh != null:
				var m = inst.mesh
				inst.free()
				return m
			for child in inst.get_children():
				if child is MeshInstance3D and child.mesh != null:
					var m = child.mesh
					inst.free()
					return m
			inst.free()
		elif res is Mesh:
			return res
	return fallback_mesh

func setup_fly_multimesh() -> void:
	if not fly_multimesh:
		return
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.instance_count = 100
	
	# Load Blender-generated biological fly mesh, or fallback to primitive sphere
	var sphere := SphereMesh.new()
	sphere.radius = 0.25
	sphere.height = 0.6
	var mesh_to_use = load_mesh_from_scene("res://assets/models/fly_agent.glb", sphere)
	mm.mesh = mesh_to_use
	fly_multimesh.multimesh = mm
	if god_fly_mesh:
		god_fly_mesh.mesh = mesh_to_use

func setup_quad_view_presentation() -> void:
	# Programmatic QuadView setup to display 4 camera viewports simultaneously
	var ui_node = get_node_or_null("UI")
	if not ui_node or quad_container != null:
		return

	quad_container = Control.new()
	quad_container.name = "QuadViewPresentation"
	quad_container.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	quad_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	ui_node.add_child(quad_container)
	ui_node.move_child(quad_container, 0) # Place behind HUD controls

	var grid = GridContainer.new()
	grid.name = "QuadGrid"
	grid.columns = 2
	grid.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	grid.add_theme_constant_override("h_separation", 4)
	grid.add_theme_constant_override("v_separation", 4)
	quad_container.add_child(grid)

	var labels := [
		"CAM 1: GOD FLY [TRACKING]",
		"CAM 2: COLONY POV [NEST]",
		"CAM 3: EVENT FOCUS [ACTION]",
		"CAM 4: WORLD OVERVIEW [WIDE]"
	]

	quad_sub_cams.clear()
	for i in range(4):
		var cont = SubViewportContainer.new()
		cont.name = "ViewportCont_%d" % i
		cont.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		cont.size_flags_vertical = Control.SIZE_EXPAND_FILL
		cont.stretch = true

		var vp = SubViewport.new()
		vp.name = "SubViewport"
		vp.own_world_3d = false # Shares main world 3D scene
		cont.add_child(vp)

		var sub_cam = Camera3D.new()
		sub_cam.name = "SubCam_%d" % i
		sub_cam.current = true
		vp.add_child(sub_cam)
		quad_sub_cams.append(sub_cam)

		# Camera channel badge overlay
		var badge = Label.new()
		badge.text = " " + labels[i] + " "
		badge.set_anchors_and_offsets_preset(Control.PRESET_TOP_LEFT)
		badge.position = Vector2(8, 8)
		cont.add_child(badge)

		grid.add_child(cont)

func set_view_mode(mode: int) -> void:
	current_view_mode = clampi(mode, 0, 4)
	if quad_container:
		quad_container.visible = (current_view_mode == 0)

	# In solo modes (1-4), activate the corresponding camera in CameraRig
	if current_view_mode == 1 and camera_godfly:
		camera_godfly.current = true
	elif current_view_mode == 2 and camera_colony:
		camera_colony.current = true
	elif current_view_mode == 3 and camera_event:
		camera_event.current = true
	elif (current_view_mode == 4 or current_view_mode == 0) and camera_global:
		camera_global.current = true

	print("[FLGODTVController] View Mode switched to: ", get_view_mode_name(current_view_mode))

func get_view_mode_name(mode: int) -> String:
	match mode:
		0: return "Quad-View (4-Camera)"
		1: return "Cam 1 (God Fly Solo)"
		2: return "Cam 2 (Colony Solo)"
		3: return "Cam 3 (Event Solo)"
		4: return "Cam 4 (Global Wide Solo)"
		_: return "Unknown"

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_0, KEY_QUOTELEFT:
				set_view_mode(0)
			KEY_1:
				set_view_mode(1)
			KEY_2:
				set_view_mode(2)
			KEY_3:
				set_view_mode(3)
			KEY_4:
				set_view_mode(4)
			KEY_B:
				is_auto_broadcast = not is_auto_broadcast
				print("[FLGODTVController] Autonomous Broadcast Director: ", "ON" if is_auto_broadcast else "OFF")
			KEY_M:
				if map_system:
					map_system.toggle_expanded()
			KEY_L:
				if map_system:
					map_system.cycle_layers()
			KEY_R:
				var hud = get_node_or_null("UI")
				if hud and hud.has_method("toggle_research_panel"):
					hud.toggle_research_panel()
			KEY_H:
				var hud = get_node_or_null("UI")
				if hud:
					var p = hud.get_node_or_null("Panel")
					if p: p.visible = not p.visible
					if map_system:
						map_system.visible = p.visible if p else true

func _process(delta: float) -> void:
	if is_auto_broadcast:
		update_broadcast_director(delta)
	update_cameras(delta)
	update_fly_instances()
	update_quad_sub_cameras()
	update_map_camera_state()

func update_broadcast_director(delta: float) -> void:
	broadcast_hold_timer -= delta
	if broadcast_hold_timer > 0.0:
		return

	# Deterministic autonomous director rules (GEMINI.md Sections 83 & 89):
	# Check for breaking events
	if bridge and bridge.events_data.size() > 0:
		var top_ev: Dictionary = bridge.events_data[0]
		var prio: float = float(top_ev.get("priority", 0.0))
		if prio >= 80.0 and current_view_mode != 3:
			set_view_mode(3) # Action event cut
			broadcast_hold_timer = 6.0
			return

	# Sequence through documentary perspectives
	match current_view_mode:
		0:
			set_view_mode(1) # God Fly Tracking
			broadcast_hold_timer = 8.0
		1:
			set_view_mode(2) # Colony POV
			broadcast_hold_timer = 7.0
		2:
			set_view_mode(4) # Archipelago Overview
			broadcast_hold_timer = 10.0
		_:
			set_view_mode(0) # 4-Camera QuadView Broadcast
			broadcast_hold_timer = 8.0

func update_map_camera_state() -> void:
	if not map_system:
		return
	var active_cam: Camera3D = camera_global
	match current_view_mode:
		1: active_cam = camera_godfly
		2: active_cam = camera_colony
		3: active_cam = camera_event
		_: active_cam = camera_global
	if active_cam and active_cam.is_inside_tree():
		map_system.active_camera_pos = active_cam.global_position
		map_system.active_camera_look = active_cam.global_position - active_cam.global_transform.basis.z * 15.0

func update_quad_sub_cameras() -> void:
	if not quad_container or not quad_container.visible or quad_sub_cams.size() < 4:
		return
	if camera_godfly and camera_godfly.is_inside_tree() and quad_sub_cams[0]:
		quad_sub_cams[0].global_transform = camera_godfly.global_transform
		quad_sub_cams[0].fov = camera_godfly.fov
	if camera_colony and camera_colony.is_inside_tree() and quad_sub_cams[1]:
		quad_sub_cams[1].global_transform = camera_colony.global_transform
		quad_sub_cams[1].fov = camera_colony.fov
	if camera_event and camera_event.is_inside_tree() and quad_sub_cams[2]:
		quad_sub_cams[2].global_transform = camera_event.global_transform
		quad_sub_cams[2].fov = camera_event.fov
	if camera_global and camera_global.is_inside_tree() and quad_sub_cams[3]:
		quad_sub_cams[3].global_transform = camera_global.global_transform
		quad_sub_cams[3].fov = camera_global.fov

func _on_state_updated(_state: Dictionary) -> void:
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
			var pos_arr: Array = pose.get("pos", [30, 55, 115])
			var look_arr: Array = pose.get("look_at", [30, 5, 25])
			var fov_val: float = pose.get("fov", 60.0)

			var target_pos := Vector3(pos_arr[0], pos_arr[1], pos_arr[2])
			var target_look := Vector3(look_arr[0], look_arr[1], look_arr[2])

			match ch_id:
				0: # Cam1_GodFly
					if camera_godfly:
						var new_pos = camera_godfly.position.lerp(target_pos, delta * 5.0)
						camera_godfly.look_at_from_position(new_pos, target_look, Vector3.UP)
						camera_godfly.fov = fov_val
				1: # Cam2_Colony / Learner
					if camera_colony:
						var new_pos = camera_colony.position.lerp(target_pos, delta * 3.5)
						camera_colony.look_at_from_position(new_pos, target_look, Vector3.UP)
						camera_colony.fov = fov_val
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

	# Deterministic safe fallback camera framing
	if camera_global:
		camera_global.look_at_from_position(Vector3(30, 55, 115), Vector3(30, 5, 25), Vector3.UP)
		camera_global.fov = 60.0

	if camera_godfly and bridge.god_fly_data.has("position"):
		var gf_pos: Vector3 = bridge.god_fly_data["position"]
		var target_pos := gf_pos + Vector3(0, 3.5, 6.0)
		var new_pos = camera_godfly.position.lerp(target_pos, delta * 4.0)
		camera_godfly.look_at_from_position(new_pos, gf_pos, Vector3.UP)

	if camera_colony:
		var nest_pos: Vector3 = bridge.colonies_data[0].get("nest", Vector3.ZERO) if bridge.colonies_data.size() > 0 else Vector3.ZERO
		camera_colony.look_at_from_position(nest_pos + Vector3(12, 10, 16), nest_pos + Vector3(0, 1, 0), Vector3.UP)

	if camera_event:
		var focus: Vector3 = Vector3(20, 2, 20)
		if bridge.agents_data.size() > 0:
			focus = bridge.agents_data[0].get("position", focus)
		var new_pos = camera_event.position.lerp(focus + Vector3(3.0, 2.5, 4.5), delta * 2.5)
		camera_event.look_at_from_position(new_pos, focus, Vector3.UP)

func update_fly_instances() -> void:
	if not fly_multimesh or not fly_multimesh.multimesh or not bridge:
		return
	var mm := fly_multimesh.multimesh
	var agents: Array = bridge.agents_data
	var count : int = min(agents.size(), mm.instance_count)
	var time_now: float = bridge.simulation_time if bridge else 0.0

	for i in range(count):
		var a: Dictionary = agents[i]
		var pos: Vector3 = a.get("position", Vector3.ZERO)
		var vel: Vector3 = a.get("velocity", Vector3.ZERO)
		var action: String = a.get("action", "Fly")
		var cid: int = a.get("colony_id", 1)

		# Procedural high-frequency wing oscillation and micro-bobbing (Agent V2)
		var wing_bob: float = sin(time_now * 28.0 + float(i) * 1.7) * 0.04
		var current_pos := pos + Vector3(0.0, wing_bob, 0.0)

		var t := Transform3D()
		if vel.length_squared() > 0.05:
			var forward := vel.normalized()
			var up := Vector3.UP
			var right := forward.cross(up).normalized()
			up = right.cross(forward).normalized()

			# Subtle flight bank angle based on lateral turn
			var bank_angle: float = clampf(vel.x * 0.15, -0.4, 0.4)
			var base_basis := Basis(right, up, -forward)
			t.basis = base_basis.rotated(forward, bank_angle)
		else:
			# Stationary / Resting pose
			t.basis = Basis.IDENTITY

		# Scale modulation based on action
		var scale_val: float = 1.0
		if action == "Rest":
			scale_val = 0.85
		elif action == "Feed" or action == "Construct":
			scale_val = 1.1

		t.basis = t.basis.scaled(Vector3(scale_val, scale_val, scale_val))
		t.origin = current_pos
		mm.set_instance_transform(i, t)

		# Action and colony aware color coding (Visual V2)
		var base_col: Color = Color(0.2, 0.85, 1.0) if cid == 1 else Color(1.0, 0.65, 0.18)
		var display_col: Color = base_col
		match action:
			"Forage":
				display_col = base_col.lerp(Color(0.4, 1.0, 0.4), 0.3)
			"Feed":
				display_col = Color(1.0, 0.9, 0.25) # Nectar feeding gold
			"Interact":
				display_col = Color(0.9, 0.4, 0.9) # Social cohesion magenta
			"Construct":
				display_col = Color(0.95, 0.5, 0.3) # Technology/construction amber
			"Rest":
				display_col = base_col.darkened(0.3)

		mm.set_instance_color(i, display_col)

	# Update God Fly visual mesh with smooth teacher orbit
	if god_fly_mesh and bridge.god_fly_data.has("position"):
		god_fly_mesh.position = bridge.god_fly_data["position"]
		# Subtle golden scale pulse indicating active teaching
		var pulse: float = 1.0 + sin(time_now * 2.0) * 0.06
		god_fly_mesh.scale = Vector3(pulse, pulse, pulse)
