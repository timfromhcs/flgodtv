extends Node3D
class_name WeatherRenderer

# WeatherRenderer visualizes backend weather consequences per GEMINI.md
# Section 75. Backend owns rain intensity/wind/visibility/humidity/temperature;
# Godot renders clouds/rain/fog/wet-surface cues only. Unknown values display
# as unknown (particles off, N/A flag) — never invented (Section 85).

var rain_particles: GPUParticles3D
var fog_volume_note: String = ""
var last_weather: Dictionary = {}
var weather_known: bool = false

func ensure_initialized() -> void:
	if rain_particles != null:
		return
	rain_particles = GPUParticles3D.new()
	rain_particles.name = "RainParticles"
	rain_particles.amount = 2048
	rain_particles.lifetime = 1.2
	rain_particles.visibility_aabb = AABB(Vector3(-60, -5, -60), Vector3(120, 30, 120))
	var pm := ParticleProcessMaterial.new()
	pm.direction = Vector3(0, -1, 0)
	pm.spread = 8.0
	pm.initial_velocity_min = 18.0
	pm.initial_velocity_max = 26.0
	pm.gravity = Vector3(0, -9.8, 0)
	rain_particles.process_material = pm
	var quad := QuadMesh.new()
	quad.size = Vector2(0.03, 0.35)
	var qmat := StandardMaterial3D.new()
	qmat.albedo_color = Color(0.6, 0.75, 0.95, 0.55)
	qmat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	quad.material = qmat
	rain_particles.draw_pass_1 = quad
	add_child(rain_particles)
	rain_particles.emitting = false

func _ready() -> void:
	ensure_initialized()

## Consume canonical backend weather. Expected keys (all optional):
## {"precipitation": float 0..1, "visibility": float, "wind": Vector2,
##  "temperature_c": float, "humidity": float}. Missing keys -> unknown.
func apply_backend_weather(w: Dictionary) -> void:
	last_weather = w
	var has_any: bool = w.has("precipitation") or w.has("visibility") or w.has("wind")
	weather_known = has_any
	if not has_any:
		rain_particles.emitting = false
		return
	var precip: float = float(w.get("precipitation", 0.0))
	rain_particles.emitting = precip > 0.05
	if rain_particles.emitting:
		rain_particles.amount = int(clampf(precip * 4096.0, 256.0, 4096.0))
	var env := get_world_env()
	if env and env.environment:
		var vis: float = float(w.get("visibility", 1000.0))
		# Map visibility 50..1000m to fog density cue (visual only).
		var fog_on: bool = vis < 800.0
		env.environment.fog_enabled = fog_on
		if fog_on:
			env.environment.fog_light_color = Color(0.7, 0.75, 0.8)
			env.environment.fog_density = clampf((800.0 - vis) / 800.0 * 0.02, 0.0005, 0.02)

func get_world_env() -> WorldEnvironment:
	if get_parent() and get_parent().has_node("WorldEnvironment"):
		return get_parent().get_node("WorldEnvironment") as WorldEnvironment
	var root := get_tree().current_scene if is_inside_tree() and get_tree() else null
	if root and root.has_node("WorldEnvironment"):
		return root.get_node_or_null("WorldEnvironment") as WorldEnvironment
	return null

func is_weather_known() -> bool:
	return weather_known

func get_summary() -> String:
	if not weather_known:
		return "N/A"
	return "precip=%.2f vis=%.0f" % [float(last_weather.get("precipitation", 0.0)), float(last_weather.get("visibility", 1000.0))]
