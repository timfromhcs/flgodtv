extends Node3D
class_name WaterRenderer

# WaterRenderer renders the water surface visualizing backend canonical water
# state (height/flow) per GEMINI.md Section 74 & Visual V2 Target.
# Backend owns the canonical water field; Godot provides a physically inspired
# visual approximation (surface refraction, flow current cues, and shoreline foam).
# Use set_water_level() or apply_backend_state() to mirror backend truth.

@export var water_level: float = -0.5
@export var surface_size: float = 350.0
@export var center_position: Vector3 = Vector3(30.0, 0.0, 30.0)

var mesh_instance: MeshInstance3D
var foam_mesh_instance: MeshInstance3D
var _water_material: StandardMaterial3D
var _foam_material: StandardMaterial3D
var _using_backend_data: bool = false
var _time: float = 0.0
var _flow_velocity: Vector2 = Vector2(0.04, 0.02)

func ensure_initialized() -> void:
	if mesh_instance != null:
		return
	mesh_instance = MeshInstance3D.new()
	mesh_instance.name = "WaterSurface"
	add_child(mesh_instance)

	foam_mesh_instance = MeshInstance3D.new()
	foam_mesh_instance.name = "ShorelineFoam"
	add_child(foam_mesh_instance)

	setup_water_plane()

func _ready() -> void:
	ensure_initialized()

func setup_water_plane() -> void:
	var plane := PlaneMesh.new()
	plane.size = Vector2(surface_size, surface_size)
	plane.subdivide_width = 48
	plane.subdivide_depth = 48

	_water_material = StandardMaterial3D.new()
	_water_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_water_material.albedo_color = Color(0.10, 0.42, 0.62, 0.78)
	_water_material.roughness = 0.12
	_water_material.metallic = 0.15
	_water_material.metallic_specular = 0.55
	_water_material.refraction_enabled = true
	_water_material.refraction_scale = 0.03
	plane.material = _water_material

	mesh_instance.mesh = plane
	mesh_instance.position = Vector3(center_position.x, water_level, center_position.z)

	# Water V2: Shoreline Foam Fringe layer
	var foam_plane := PlaneMesh.new()
	foam_plane.size = Vector2(surface_size * 0.98, surface_size * 0.98)
	foam_plane.subdivide_width = 32
	foam_plane.subdivide_depth = 32

	_foam_material = StandardMaterial3D.new()
	_foam_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_foam_material.albedo_color = Color(0.85, 0.92, 0.98, 0.18)
	_foam_material.roughness = 0.45
	_foam_material.metallic_specular = 0.2
	foam_plane.material = _foam_material

	foam_mesh_instance.mesh = foam_plane
	foam_mesh_instance.position = Vector3(center_position.x, water_level + 0.04, center_position.z)

	print("[WaterRenderer] Water V2 surface at Y=" + str(water_level) + " backend=" + str(_using_backend_data) + ".")

func set_water_level(h: float) -> void:
	water_level = h
	if mesh_instance:
		mesh_instance.position.y = h
	if foam_mesh_instance:
		foam_mesh_instance.position.y = h + 0.04

## Consume canonical backend water state.
## Expected keys: {"height": float, "flow_x": float, "flow_y": float}
func apply_backend_state(state: Dictionary) -> bool:
	if not state.has("height"):
		return false
	_using_backend_data = true
	set_water_level(float(state["height"]))
	if state.has("flow_x") and state.has("flow_y"):
		_flow_velocity = Vector2(float(state["flow_x"]), float(state["flow_y"])).limit_length(0.2)
	return true

func is_using_backend_data() -> bool:
	return _using_backend_data

func get_water_level() -> float:
	return water_level

func _process(delta: float) -> void:
	# Subtle visual wave swell and surface flow animation (GEMINI.md Section 74)
	_time += delta
	var swell: float = sin(_time * 1.1) * 0.035 + cos(_time * 2.2) * 0.015

	if mesh_instance and mesh_instance.mesh and mesh_instance.mesh is PlaneMesh:
		mesh_instance.position.y = water_level + swell
		if _water_material:
			_water_material.uv1_offset = Vector3(_time * _flow_velocity.x, _time * _flow_velocity.y, 0.0)

	if foam_mesh_instance and foam_mesh_instance.mesh:
		foam_mesh_instance.position.y = water_level + swell + 0.04
		if _foam_material:
			_foam_material.uv1_offset = Vector3(-_time * _flow_velocity.x * 0.5, _time * _flow_velocity.y * 0.7, 0.0)
