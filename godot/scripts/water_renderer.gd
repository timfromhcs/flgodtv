extends Node3D
class_name WaterRenderer

# WaterRenderer renders the water surface visualizing backend canonical water
# state (height/flow) per GEMINI.md Section 74. Backend owns the water field;
# Godot renders surface/reflection/foam visuals only. Use set_water_level()
# or apply_backend_state() to mirror backend; the exported default is only a
# fallback and must be reported as such in telemetry.

@export var water_level: float = -0.5
@export var surface_size: float = 120.0

var mesh_instance: MeshInstance3D
var _using_backend_data: bool = false
var _time: float = 0.0

func ensure_initialized() -> void:
	if mesh_instance != null:
		return
	mesh_instance = MeshInstance3D.new()
	mesh_instance.name = "WaterSurface"
	add_child(mesh_instance)
	setup_water_plane()

func _ready() -> void:
	ensure_initialized()

func setup_water_plane() -> void:
	var plane := PlaneMesh.new()
	plane.size = Vector2(surface_size, surface_size)
	plane.subdivide_width = 16
	plane.subdivide_depth = 16

	var mat := StandardMaterial3D.new()
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.albedo_color = Color(0.12, 0.45, 0.65, 0.75)
	mat.roughness = 0.15
	mat.metallic = 0.1
	mat.refraction_enabled = true
	mat.refraction_scale = 0.02
	plane.material = mat

	mesh_instance.mesh = plane
	mesh_instance.position = Vector3(0, water_level, 0)
	print("[WaterRenderer] Water surface at Y=" + str(water_level) + " backend=" + str(_using_backend_data) + ".")

func set_water_level(h: float) -> void:
	water_level = h
	if mesh_instance:
		mesh_instance.position.y = h

## Consume canonical backend water state. Expected: {"height": float}.
func apply_backend_state(state: Dictionary) -> bool:
	if not state.has("height"):
		return false
	_using_backend_data = true
	set_water_level(float(state["height"]))
	return true

func is_using_backend_data() -> bool:
	return _using_backend_data

func get_water_level() -> float:
	return water_level

func _process(delta: float) -> void:
	# Subtle visual ripple; simulation state itself is owned by the backend.
	_time += delta
	if mesh_instance and mesh_instance.mesh and mesh_instance.mesh is PlaneMesh:
		mesh_instance.position.y = water_level + sin(_time * 0.8) * 0.03
