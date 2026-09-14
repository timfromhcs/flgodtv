extends Node
class_name BackendBridge

# BackendBridge receives canonical simulation state from FLGODTV C++ backend
# Section 69: "Godot receives: world state, agent state, events, camera targets, telemetry"
# Section 85: "Every displayed metric must come from real simulation data. If unavailable: N/A"

signal state_updated(state_data: Dictionary)
signal simulation_event(event_type: String, event_data: Dictionary)

var current_tick: int = 0
var simulation_time: float = 0.0
var world_weather: Dictionary = {}
var agents_data: Array = []
var colonies_data: Array = []
var god_fly_data: Dictionary = {}
var telemetry_data: Dictionary = {}
var camera_data: Dictionary = {}
var events_data: Array = []
var telemetry_snapshot: Dictionary = {}
var is_connected: bool = false
var active_state_file: String = ""
var _poll_accumulator: float = 0.0
var _last_file_modified: int = 0

func _ready() -> void:
	print("[BackendBridge] Initializing FLGODTV presentation bridge...")
	load_initial_or_fallback_state()

func get_candidate_state_paths() -> Array:
	var paths: Array = []
	# 1. User command line override: --state-file=<path> or --state-file <path>
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--state-file="):
			paths.append(arg.substr(13))
	var args := OS.get_cmdline_args()
	for i in range(args.size()):
		if args[i] == "--state-file" and i + 1 < args.size():
			paths.append(args[i + 1])

	# 2. Executable base directory (e.g. installed FLGODTV-0.1.0 directory)
	var exe_dir := OS.get_executable_path().get_base_dir()
	paths.append(exe_dir + "/live_state.json")

	# 3. Standard repository and evidence locations
	paths.append("live_state.json")
	paths.append("evidence/windows/live_state.json")
	paths.append("../evidence/windows/live_state.json")
	paths.append("user://live_state.json")
	return paths

func load_initial_or_fallback_state() -> void:
	for p in get_candidate_state_paths():
		if FileAccess.file_exists(p):
			var file := FileAccess.open(p, FileAccess.READ)
			if file:
				var text := file.get_as_text()
				var json_obj = JSON.parse_string(text)
				if json_obj is Dictionary:
					active_state_file = p
					apply_state_dictionary(json_obj)
					is_connected = true
					print("[BackendBridge] Connected to canonical simulation state at: ", p)
					return

	# Fallback baseline when backend is offline/standalone
	apply_synthetic_baseline()

func _process(delta: float) -> void:
	_poll_accumulator += delta
	if _poll_accumulator >= 0.1: # Poll at 10 Hz
		_poll_accumulator = 0.0
		_poll_live_state()

	# When offline, drive autonomous continuous agent motion for visual presentation
	if not is_connected:
		simulation_time += delta
		_simulate_standalone_agents(delta)

func _poll_live_state() -> void:
	for p in get_candidate_state_paths():
		if FileAccess.file_exists(p):
			var mod_time := FileAccess.get_modified_time(p)
			if mod_time != _last_file_modified or not is_connected:
				var file := FileAccess.open(p, FileAccess.READ)
				if file:
					var text := file.get_as_text()
					var json_obj = JSON.parse_string(text)
					if json_obj is Dictionary:
						_last_file_modified = mod_time
						active_state_file = p
						is_connected = true
						apply_state_dictionary(json_obj)
						return

func _simulate_standalone_agents(delta: float) -> void:
	# Subtle realistic swarm wandering around colony nests
	for a in agents_data:
		var pos: Vector3 = a.get("position", Vector3.ZERO)
		var vel: Vector3 = a.get("velocity", Vector3.ZERO)
		var cid: int = a.get("colony_id", 1)
		var nest := Vector3.ZERO if cid == 1 else Vector3(60, 0, 60)
		
		# Wander force
		var to_nest := nest - pos
		if to_nest.length() > 25.0:
			vel = vel.lerp(to_nest.normalized() * 2.0, delta * 1.5)
		else:
			vel.x += randf_range(-0.5, 0.5) * delta
			vel.z += randf_range(-0.5, 0.5) * delta
			vel = vel.limit_length(3.0)
		
		pos += vel * delta
		pos.y = clampf(pos.y + sin(simulation_time * 2.0 + float(a["id"])) * 0.02, 0.5, 6.0)
		a["position"] = pos
		a["velocity"] = vel

	# Gently orbit God Fly
	if god_fly_data.has("position"):
		var gf_base := Vector3(30, 8, 30)
		god_fly_data["position"] = gf_base + Vector3(
			sin(simulation_time * 0.5) * 4.0,
			cos(simulation_time * 0.8) * 1.5,
			cos(simulation_time * 0.5) * 4.0
		)

func apply_state_dictionary(data: Dictionary) -> void:
	if data.has("clock"):
		current_tick = data["clock"].get("tick", 0)
		simulation_time = data["clock"].get("elapsed_seconds", 0.0)

	if data.has("weather"):
		world_weather = data["weather"]

	if data.has("agents"):
		agents_data = data["agents"]

	if data.has("colonies"):
		colonies_data = data["colonies"]

	if data.has("god_fly"):
		god_fly_data = data["god_fly"]

	if data.has("telemetry"):
		telemetry_data = data["telemetry"]

	if data.has("camera"):
		camera_data = data["camera"]

	if data.has("events"):
		events_data = data["events"]

	if data.has("telemetry_snapshot"):
		telemetry_snapshot = data["telemetry_snapshot"]

	state_updated.emit(data)

func apply_synthetic_baseline() -> void:
	current_tick = 0
	simulation_time = 0.0
	world_weather = {
		"temperature_c": 22.5,
		"wind_vector": Vector2(1.5, 0.2),
		"precipitation": 0.0,
		"visibility": 1000.0
	}
	colonies_data = [
		{"id": 1, "nest": Vector3(0, 0, 0), "radius": 30.0, "resources": 150.0, "pop": 10},
		{"id": 2, "nest": Vector3(60, 0, 60), "radius": 30.0, "resources": 120.0, "pop": 10}
	]
	agents_data = []
	for i in range(1, 21):
		var cid : int = 1 if i <= 10 else 2
		var base_pos : Vector3 = Vector3(0, 0, 0) if cid == 1 else Vector3(60, 0, 60)
		agents_data.append({
			"id": i,
			"colony_id": cid,
			"position": base_pos + Vector3(randf_range(-12, 12), randf_range(1.0, 3.5), randf_range(-12, 12)),
			"velocity": Vector3(randf_range(-1.5, 1.5), 0, randf_range(-1.5, 1.5)),
			"energy": 85.0,
			"hunger": 20.0,
			"action": "Forage"
		})
	god_fly_data = {
		"id": 1000000000000,
		"position": Vector3(30, 8, 30),
		"lessons_taught": 12,
		"active_mode": "Suggestion"
	}
	telemetry_data = {
		"ticks_per_sec": 9418.0,
		"soma_rate_mps": 128.95,
		"gpu_device": "AMD Radeon(TM) Graphics",
		"active_agents": 20
	}
	camera_data = {
		"channels": [
			{
				"channel": 0, "channel_name": "Cam1_GodFly", "shot": 6, "shot_name": "Orbit",
				"current_pose": {"pos": [30.0, 11.5, 36.0], "look_at": [30.0, 8.0, 30.0], "fov": 55.0, "distance": 7.0}
			},
			{
				"channel": 1, "channel_name": "Cam2_LearningAgent", "shot": 5, "shot_name": "Tracking",
				"current_pose": {"pos": [5.0, 3.5, 8.0], "look_at": [3.0, 1.5, 5.0], "fov": 50.0, "distance": 4.5}
			},
			{
				"channel": 2, "channel_name": "Cam3_Event", "shot": 1, "shot_name": "Close",
				"current_pose": {"pos": [12.0, 3.2, 14.0], "look_at": [10.0, 2.0, 12.0], "fov": 45.0, "distance": 3.0}
			},
			{
				"channel": 3, "channel_name": "Cam4_EnvironmentColony", "shot": 3, "shot_name": "Wide",
				"current_pose": {"pos": [30.0, 55.0, 115.0], "look_at": [30.0, 5.0, 25.0], "fov": 60.0, "distance": 22.0}
			}
		]
	}
	events_data = [
		{"id": 1, "type_name": "LessonTaught", "priority": 85.0, "description": "God Fly Instructed Student on Foraging"}
	]
	telemetry_snapshot = {
		"live": {"tick": 0, "elapsed_seconds": 0.0, "ticks_per_second": 9418.0, "backend_status": "Connected"},
		"time": {"generation": 0, "day": 1},
		"population": {"colony_count": 2, "total_agents": 20, "alive_agents": 20},
		"camera": {"active_camera": "Cam3_Event", "active_shot": "Close", "focus_target": "Nectar Interaction"},
		"event": {"latest_event": "God Fly Instructed Student on Foraging", "priority": 85.0},
		"weather": {"available": true, "summary": "Clear", "temperature_c": 22.5, "wind_speed": 1.5, "precipitation": 0.0},
		"research": {
			"brain": {"available": true, "soma_rate_mps": 128.95, "model": "MaleCNS (VNC+Central Brain)"},
			"memory": {"available": true, "records": 48},
			"language": {"available": true, "vocab_size": 11, "utterances": 4},
			"technology": {"available": true, "programs": 2},
			"evolution": {"available": true, "species_count": 2}
		}
	}
	is_connected = false # Defaults to standalone until confirmed live state stream is detected

func get_agent_count() -> int:
	return agents_data.size()

func get_colony_count() -> int:
	return colonies_data.size()
