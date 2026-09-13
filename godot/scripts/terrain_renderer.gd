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

@export var chunk_size: int = 32
@export var cell_size: float = 2.0
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
	mat.roughness = 0.85
	surface_tool.set_material(mat)

	var step := lod_step()
	var half_size: float = (chunk_size - 1) * cell_size * 0.5
	vertex_count = 0

	for z in range(0, chunk_size - 1, step):
		for x in range(0, chunk_size - 1, step):
			var x1i: int = mini(x + step, chunk_size - 1)
			var z1i: int = mini(z + step, chunk_size - 1)
			var x0: float = x * cell_size - half_size
			var x1: float = x1i * cell_size - half_size
			var z0: float = z * cell_size - half_size
			var z1: float = z1i * cell_size - half_size

			var y00: float = sample_height_grid(x, z, x0, z0)
			var y10: float = sample_height_grid(x1i, z, x1, z0)
			var y01: float = sample_height_grid(x, z1i, x0, z1)
			var y11: float = sample_height_grid(x1i, z1i, x1, z1)

			var v00 := Vector3(x0, y00, z0)
			var v10 := Vector3(x1, y10, z0)
			var v01 := Vector3(x0, y01, z1)
			var v11 := Vector3(x1, y11, z1)

			var c00 := get_biome_color(y00)
			var c10 := get_biome_color(y10)
			var c01 := get_biome_color(y01)
			var c11 := get_biome_color(y11)

			var n1: Vector3 = (v10 - v00).cross(v01 - v00).normalized()
			var n2: Vector3 = (v11 - v10).cross(v01 - v10).normalized()
			if n1.length() < 0.5:
				n1 = Vector3.UP
			if n2.length() < 0.5:
				n2 = Vector3.UP

			surface_tool.set_normal(n1)
			surface_tool.set_color(c00)
			surface_tool.add_vertex(v00)

			surface_tool.set_normal(n1)
			surface_tool.set_color(c10)
			surface_tool.add_vertex(v10)

			surface_tool.set_normal(n1)
			surface_tool.set_color(c01)
			surface_tool.add_vertex(v01)

			surface_tool.set_normal(n2)
			surface_tool.set_color(c10)
			surface_tool.add_vertex(v10)

			surface_tool.set_normal(n2)
			surface_tool.set_color(c11)
			surface_tool.add_vertex(v11)

			surface_tool.set_normal(n2)
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
	var d: float = Vector2(focus_pos.x, focus_pos.z).length()
	var visible_now: bool = d <= stream_radius
	if mesh_instance:
		mesh_instance.visible = visible_now
	var cell := Vector2i(int(floor(focus_pos.x / 64.0)), int(floor(focus_pos.z / 64.0)))
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

static func get_biome_color(elevation: float) -> Color:
	# Biome coloration matching Whittaker diagram bands used by backend.
	if elevation < -1.0:
		return Color(0.76, 0.70, 0.50) # Sandy Shoreline
	elif elevation < 2.0:
		return Color(0.35, 0.65, 0.25) # Grassland / Meadow
	elif elevation < 4.0:
		return Color(0.20, 0.48, 0.20) # Forest / Woodland
	else:
		return Color(0.55, 0.55, 0.55) # Rocky Peak
