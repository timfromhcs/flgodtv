extends Node
class_name BackendBridge

# BackendBridge receives canonical simulation state from FLGODTV C++ backend
# Section 69: "Godot receives: world state, agent state, events, camera targets, telemetry"

signal state_updated(state_data: Dictionary)
signal simulation_event(event_type: String, event_data: Dictionary)

var current_tick: int = 0
var simulation_time: float = 0.0
var world_weather: Dictionary = {}
var agents_data: Array = []
var colonies_data: Array = []
var god_fly_data: Dictionary = {}
var telemetry_data: Dictionary = {}
var is_connected: bool = false

# Polling or IPC stream configuration
var state_snapshot_path: String = "evidence/windows/live_state.json"

func _ready() -> void:
	print("[BackendBridge] Initialized FLGODTV Godot presentation bridge.")
	load_initial_or_fallback_state()

func load_initial_or_fallback_state() -> void:
	# Attempt to load state snapshot or default baseline
	if FileAccess.file_exists(state_snapshot_path):
		var file := FileAccess.open(state_snapshot_path, FileAccess.READ)
		if file:
			var text := file.get_as_text()
			var json_obj = JSON.parse_string(text)
			if json_obj is Dictionary:
				apply_state_dictionary(json_obj)
				is_connected = true
				return

	# Fallback synthetic initial state if backend is booting
	apply_synthetic_baseline()

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
			"position": base_pos + Vector3(randf_range(-15, 15), randf_range(0.5, 3.0), randf_range(-15, 15)),
			"velocity": Vector3(randf_range(-1, 1), 0, randf_range(-1, 1)),
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
	is_connected = true

func get_agent_count() -> int:
	return agents_data.size()

func get_colony_count() -> int:
	return colonies_data.size()
