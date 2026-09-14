extends Node3D
class_name TerrainRenderer

# TerrainRenderer builds procedural terrain meshes visualizing backend canonical
# elevation/biome state per GEMINI.md Sections 71 & 72.
# Backend (C++ procedural_generator.hpp) remains the canonical simulation state;
# this renderer visualizes it. sample_height() is a documented visual
# approximation of the backend fBm elevation (sin-based, same constants as
# VegetationRenderer). When the bridge supplies a `terrain_grid` dictionary
# (rows/cols/cell/elevations PackedFloat32Array in row-major order), the mesh
# is built from that canonical grid instead (see apply_backend_state()).

@export var chunk_size: int = 48
@export var cell_size: float = 2.5
@export var center_offset: Vector3 = Vector3(30.0, 0.0, 30.0)
## LOD level: 0 = full (chunk_size), 1 = half, 2 = quarter resolution.
@export var lod_level: int = 0
## Streaming radius in world units; mesh hides when camera target is beyond it.
@export var stream_radius: float = 220.0

var mesh_instance: MeshInstance3D
var _using_backend_grid: bool = false
var _backend_grid: Dictionary = {}
var _last_stream_cell: Vector2i = Vector2i(999999, 999999)
var vertex_count: int = 0

func ensure_initialized() -> void:
	if mesh_instance != null:
		return
	mesh_instance = MeshInstance3D.new()
	mesh_instance.name = "TerrainMesh"
	add_child(mesh_instance)
	generate_terrain_mesh()

func _ready() -> void:
	ensure_initialized()

func lod_step() -> int:
	return int(pow(2.0, float(clampi(lod_level, 0, 2))))

func effective_resolution() -> int:
	var step := lod_step()
	return int(ceil(float(chunk_size) / float(step)))

func generate_terrain_mesh() -> void:
	var surface_tool := SurfaceTool.new()
	surface_tool.begin(Mesh.PRIMITIVE_TRIANGLES)

	var mat := StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	mat.roughness = 0.82
	mat.metallic_specular = 0.25
	surface_tool.set_material(mat)

	var step := lod_step()
	var half_size: float = (chunk_size - 1) * cell_size * 0.5
	vertex_count = 0

	for z in range(0, chunk_size - 1, step):
		for x in range(0, chunk_size - 1, step):
			var x1i: int = mini(x + step, chunk_size - 1)
			var z1i: int = mini(z + step, chunk_size - 1)
			var x0: float = center_offset.x + x * cell_size - half_size
			var x1: float = center_offset.x + x1i * cell_size - half_size
			var z0: float = center_offset.z + z * cell_size - half_size
			var z1: float = center_offset.z + z1i * cell_size - half_size

			var y00: float = sample_height_grid(x, z, x0, z0)
			var y10: float = sample_height_grid(x1i, z, x1, z0)
			var y01: float = sample_height_grid(x, z1i, x0, z1)
			var y11: float = sample_height_grid(x1i, z1i, x1, z1)

			var v00 := Vector3(x0, y00, z0)
			var v10 := Vector3(x1, y10, z0)
			var v01 := Vector3(x0, y01, z1)
			var v11 := Vector3(x1, y11, z1)

			# Central-difference smooth normal calculation (Terrain V2)
			var n00 := compute_vertex_normal(x, z, x0, z0, step)
			var n10 := compute_vertex_normal(x1i, z, x1, z0, step)
			var n01 := compute_vertex_normal(x, z1i, x0, z1, step)
			var n11 := compute_vertex_normal(x1i, z1i, x1, z1, step)

			var c00 := get_biome_color_v2(y00, n00)
			var c10 := get_biome_color_v2(y10, n10)
			var c01 := get_biome_color_v2(y01, n01)
			var c11 := get_biome_color_v2(y11, n11)

			# Tri 1: v00 -> v10 -> v01
			surface_tool.set_normal(n00)
			surface_tool.set_color(c00)
			surface_tool.add_vertex(v00)

			surface_tool.set_normal(n10)
			surface_tool.set_color(c10)
			surface_tool.add_vertex(v10)

			surface_tool.set_normal(n01)
			surface_tool.set_color(c01)
			surface_tool.add_vertex(v01)

			# Tri 2: v10 -> v11 -> v01
			surface_tool.set_normal(n10)
			surface_tool.set_color(c10)
			surface_tool.add_vertex(v10)

			surface_tool.set_normal(n11)
			surface_tool.set_color(c11)
			surface_tool.add_vertex(v11)

			surface_tool.set_normal(n01)
			surface_tool.set_color(c01)
			surface_tool.add_vertex(v01)
			vertex_count += 6

	var array_mesh: ArrayMesh = surface_tool.commit()
	mesh_instance.mesh = array_mesh
	print("[TerrainRenderer] Terrain mesh: LOD%d step=%d verts=%d backend_grid=%s." % [lod_level, step, vertex_count, str(_using_backend_grid)])

func set_lod(level: int) -> void:
	var clamped := clampi(level, 0, 2)
	if clamped == lod_level and mesh_instance and mesh_instance.mesh:
		return
	lod_level = clamped
	generate_terrain_mesh()

## Streaming: hide mesh when the focus point is outside stream_radius, and
## record the cell so callers can skip redundant rebuilds.
func update_streaming(focus_pos: Vector3) -> bool:
	var d: float = Vector2(focus_pos.x - center_offset.x, focus_pos.z - center_offset.z).length()
	var visible_now: bool = d <= stream_radius
	if mesh_instance:
		mesh_instance.visible = visible_now
	var cell := Vector2i(int(floor((focus_pos.x - center_offset.x) / 64.0)), int(floor((focus_pos.z - center_offset.z) / 64.0)))
	var changed: bool = cell != _last_stream_cell
	_last_stream_cell = cell
	return changed

## Consume canonical backend terrain state when available.
## Expected: {"rows": int, "cols": int, "elevations": PackedFloat32Array}
func apply_backend_state(grid: Dictionary) -> bool:
	if not grid.has("rows") or not grid.has("cols") or not grid.has("elevations"):
		return false
	_using_backend_grid = true
	_backend_grid = grid
	generate_terrain_mesh()
	return true

func is_using_backend_grid() -> bool:
	return _using_backend_grid

func sample_height_grid(gx: int, gz: int, x: float, z: float) -> float:
	if _using_backend_grid:
		var rows: int = int(_backend_grid["rows"])
		var cols: int = int(_backend_grid["cols"])
		var elev: PackedFloat32Array = _backend_grid["elevations"]
		var cx: int = clampi(gx, 0, cols - 1)
		var cz: int = clampi(gz, 0, rows - 1)
		var idx: int = cz * cols + cx
		if idx >= 0 and idx < elev.size():
			return elev[idx]
	return sample_height(x, z)

static func sample_height(x: float, z: float) -> float:
	# Visual approximation of backend fBm elevation (documented, not bit-exact).
	var h1: float = sin(x * 0.05) * cos(z * 0.05) * 4.0
	var h2: float = sin(x * 0.12 + 1.2) * cos(z * 0.12 + 0.8) * 1.5
	return h1 + h2

func compute_vertex_normal(gx: int, gz: int, x: float, z: float, step: int) -> Vector3:
	var delta_coord: float = float(step) * cell_size
	var h_left: float = sample_height_grid(gx - step, gz, x - delta_coord, z)
	var h_right: float = sample_height_grid(gx + step, gz, x + delta_coord, z)
	var h_down: float = sample_height_grid(gx, gz - step, x, z - delta_coord)
	var h_up: float = sample_height_grid(gx, gz + step, x, z + delta_coord)
	var n := Vector3(h_left - h_right, 2.0 * delta_coord, h_down - h_up).normalized()
	if n.length() < 0.5 or is_nan(n.x) or is_nan(n.y) or is_nan(n.z):
		return Vector3.UP
	return n

static func get_biome_color(elevation: float) -> Color:
	return get_biome_color_v2(elevation, Vector3.UP)

static func get_biome_color_v2(elevation: float, normal: Vector3) -> Color:
	# Slope-aware biome blending (Visual V2):
	# normal.y close to 1.0 is flat; normal.y < 0.70 is steep cliff/rock.
	var slope: float = clampf(normal.y, 0.0, 1.0)
	var rock_color := Color(0.44, 0.42, 0.40) # Cliff rock / shale
	var cliff_steep_color := Color(0.32, 0.30, 0.30) # Dark bedrock

	var base_biome: Color
	if elevation < -0.4:
		# Shoreline / Wet Sand / Mud
		base_biome = Color(0.74, 0.68, 0.48)
	elif elevation < 2.2:
		# Lush Grassland / Meadow
		base_biome = Color(0.30, 0.62, 0.22)
	elif elevation < 4.8:
		# Woodland / Temperate Forest
		base_biome = Color(0.18, 0.44, 0.16)
	elif elevation < 7.0:
		# Subalpine Scree & Moss
		base_biome = Color(0.42, 0.50, 0.36)
	else:
		# Alpine Snow Summit
		base_biome = Color(0.92, 0.95, 0.98)

	# If slope is steep, blend toward rock cliff
	if slope < 0.60:
		return cliff_steep_color.lerp(rock_color, slope / 0.60)
	elif slope < 0.78:
		var factor: float = (slope - 0.60) / 0.18
		return rock_color.lerp(base_biome, factor)
	else:
		return base_biome
