extends Node
class_name AtmosphereController

# AtmosphereController coordinates the diurnal time-of-day cycle and atmospheric
# lighting progression per GEMINI.md Section 76 & Visual V2 Target.
# Driven deterministically by canonical simulation clock / bridge.simulation_time.
# Visual presentation is an artistic approximation of dawn/noon/sunset/night.

@export var day_length_seconds: float = 240.0 # 4 minutes per 24h simulation day
@export var is_diurnal_enabled: bool = true

var sun_light: DirectionalLight3D
var world_env: WorldEnvironment
var bridge: Node = null

# Diurnal phase enumeration
enum TimeOfDay { NIGHT, DAWN, NOON, GOLDEN_HOUR, SUNSET }

var current_phase: int = TimeOfDay.NOON

func _ready() -> void:
	find_references()

func find_references() -> void:
	var root := get_tree().current_scene if is_inside_tree() and get_tree() else get_parent()
	if root:
		sun_light = root.get_node_or_null("DirectionalLight3D") as DirectionalLight3D
		world_env = root.get_node_or_null("WorldEnvironment") as WorldEnvironment
		bridge = root.get_node_or_null("BackendBridge")

func _process(_delta: float) -> void:
	if not is_diurnal_enabled:
		return
	if not sun_light or not world_env:
		find_references()
		if not sun_light or not world_env:
			return

	var sim_time: float = bridge.simulation_time if bridge else 0.0
	update_diurnal_cycle(sim_time)

func update_diurnal_cycle(sim_time: float) -> void:
	var cycle_progress: float = fmod(sim_time, day_length_seconds) / day_length_seconds
	# cycle_progress: 0.0=Dawn, 0.25=Noon, 0.50=Sunset, 0.75=Midnight

	var sun_angle_deg: float = (cycle_progress * 360.0) - 90.0
	var sun_rad: float = deg_to_rad(sun_angle_deg)

	# Sun rotation: arcs across sky
	if sun_light:
		sun_light.rotation.x = -sin(sun_rad) * 0.95
		sun_light.rotation.y = cos(sun_rad) * 0.6 + 0.3

	var sky_mat: ProceduralSkyMaterial = null
	if world_env and world_env.environment and world_env.environment.sky:
		sky_mat = world_env.environment.sky.sky_material as ProceduralSkyMaterial

	# Phase evaluation
	if cycle_progress < 0.15:
		# Dawn
		current_phase = TimeOfDay.DAWN
		var t: float = cycle_progress / 0.15
		if sun_light:
			sun_light.light_color = Color(1.0, 0.65, 0.45).lerp(Color(1.0, 0.92, 0.82), t)
			sun_light.light_energy = lerpf(0.3, 1.1, t)
		if sky_mat:
			sky_mat.sky_top_color = Color(0.12, 0.22, 0.45).lerp(Color(0.25, 0.48, 0.82), t)
			sky_mat.sky_horizon_color = Color(0.85, 0.45, 0.35).lerp(Color(0.70, 0.80, 0.90), t)
	elif cycle_progress < 0.50:
		# Noon / Daylight
		current_phase = TimeOfDay.NOON
		if sun_light:
			sun_light.light_color = Color(1.0, 0.98, 0.94)
			sun_light.light_energy = 1.2
		if sky_mat:
			sky_mat.sky_top_color = Color(0.20, 0.45, 0.85)
			sky_mat.sky_horizon_color = Color(0.68, 0.78, 0.88)
	elif cycle_progress < 0.65:
		# Golden Hour / Sunset
		current_phase = TimeOfDay.SUNSET
		var t: float = (cycle_progress - 0.50) / 0.15
		if sun_light:
			sun_light.light_color = Color(1.0, 0.94, 0.82).lerp(Color(1.0, 0.45, 0.20), t)
			sun_light.light_energy = lerpf(1.2, 0.4, t)
		if sky_mat:
			sky_mat.sky_top_color = Color(0.20, 0.45, 0.85).lerp(Color(0.15, 0.12, 0.35), t)
			sky_mat.sky_horizon_color = Color(0.68, 0.78, 0.88).lerp(Color(0.92, 0.40, 0.25), t)
	else:
		# Night
		current_phase = TimeOfDay.NIGHT
		var t: float = (cycle_progress - 0.65) / 0.35
		if sun_light:
			# Moonlight
			sun_light.light_color = Color(0.35, 0.45, 0.65)
			sun_light.light_energy = 0.22
		if sky_mat:
			sky_mat.sky_top_color = Color(0.04, 0.05, 0.12)
			sky_mat.sky_horizon_color = Color(0.08, 0.12, 0.22)

func get_phase_name() -> String:
	match current_phase:
		TimeOfDay.DAWN: return "Dawn"
		TimeOfDay.NOON: return "Daylight"
		TimeOfDay.GOLDEN_HOUR: return "Golden Hour"
		TimeOfDay.SUNSET: return "Sunset"
		_: return "Night"
