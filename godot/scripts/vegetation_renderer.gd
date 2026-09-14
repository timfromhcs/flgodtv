extends Node3D
class_name VegetationRenderer

# VegetationRenderer renders vegetation using GPU-friendly MultiMesh instancing
# per GEMINI.md Section 73 ("Do not instantiate every blade/tree as an
# independent heavy scene node"). Backend owns species/position/growth/health;
# Godot only mirrors. Positions come from apply_backend_instances() when the
# bridge supplies canonical vegetation state, otherwise from the deterministic
# fallback distribution (seed fixed, no visual randomness as proof of anything).

@export var instance_count: int = 200
@export var distribution_radius: float = 65.0
@export var distribution_seed: int = 424242
@export var center_position: Vector3 = Vector3(30.0, 0.0, 30.0)

var multimesh_instance: MultiMeshInstance3D
var rock_multimesh_instance: MultiMeshInstance3D
var placed_count: int = 0
var rock_placed_count: int = 0
var _using_backend_data: bool = false

func ensure_initialized() -> void:
	if multimesh_instance != null:
		return
	multimesh_instance = MultiMeshInstance3D.new()
	multimesh_instance.name = "VegetationMultiMesh"
	add_child(multimesh_instance)

	rock_multimesh_instance = MultiMeshInstance3D.new()
	rock_multimesh_instance.name = "RockMultiMesh"
	add_child(rock_multimesh_instance)

	setup_vegetation_instances()

func _ready() -> void:
	ensure_initialized()

func setup_vegetation_instances() -> void:
	_using_backend_data = false
	var positions := deterministic_positions(instance_count, distribution_radius, distribution_seed)
	apply_positions(positions)
	setup_rock_instances()

func setup_rock_instances() -> void:
	if not rock_multimesh_instance:
		return
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.instance_count = 60

	var sphere := SphereMesh.new()
	sphere.radius = 0.6
	sphere.height = 0.8
	var rock_mesh = load_mesh_from_scene("res://assets/models/environment_rock.glb", sphere)
	mm.mesh = rock_mesh

	var rng := RandomNumberGenerator.new()
	rng.seed = distribution_seed + 10101
	rock_placed_count = 0

	for slot in range(mm.instance_count):
		var angle: float = rng.randf_range(0, TAU)
		var dist: float = rng.randf_range(8.0, distribution_radius * 0.9)
		var x: float = center_position.x + cos(angle) * dist
		var z: float = center_position.z + sin(angle) * dist
		var y: float = sample_height(x, z)
		var t := Transform3D()

		if y >= 0.2: # Rocks only on solid ground
			t.origin = Vector3(x, y + 0.3, z)
			var s: float = rng.randf_range(0.5, 1.6)
			t.basis = t.basis.scaled(Vector3(s, s * 0.7, s)).rotated(Vector3.UP, rng.randf_range(0, TAU))
			mm.set_instance_transform(slot, t)
			var shade: float = rng.randf_range(0.35, 0.55)
			mm.set_instance_color(slot, Color(shade, shade * 0.95, shade * 0.9))
			rock_placed_count += 1
		else:
			t.origin = Vector3(0, -1000, 0)
			mm.set_instance_transform(slot, t)
			mm.set_instance_color(slot, Color(0, 0, 0, 0))

	rock_multimesh_instance.multimesh = mm
	print("[VegetationRenderer] Rocks: placed=%d/%d." % [rock_placed_count, mm.instance_count])

func deterministic_positions(count: int, radius: float, seed_value: int) -> Array:
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_value
	var out: Array = []
	for i in range(count):
		var angle: float = rng.randf_range(0, TAU)
		var dist: float = rng.randf_range(5.0, radius)
		var x: float = center_position.x + cos(angle) * dist
		var z: float = center_position.z + sin(angle) * dist
		var y: float = sample_height(x, z)
		if y < -0.3:
			continue # Don't place underwater
		var scale_factor: float = rng.randf_range(0.6, 1.4)
		out.append({"pos": Vector3(x, y + 0.6, z), "scale": scale_factor, "index": i})
	return out

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

func apply_positions(items: Array) -> void:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.instance_count = maxi(instance_count, items.size())

	var cylinder := CylinderMesh.new()
	cylinder.top_radius = 0.25
	cylinder.bottom_radius = 0.08
	cylinder.height = 1.2

	var mat := StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	mat.roughness = 0.7
	cylinder.material = mat

	var flora_mesh = load_mesh_from_scene("res://assets/models/flora_shrub.glb", cylinder)
	mm.mesh = flora_mesh

	placed_count = 0
	for slot in range(mm.instance_count):
		var t := Transform3D()
		if slot < items.size():
			var item: Dictionary = items[slot]
			t.origin = item["pos"]
			var s: float = float(item.get("scale", 1.0))
			t.basis = t.basis.scaled(Vector3(s, s, s))
			mm.set_instance_transform(slot, t)
			var idx: int = int(item.get("index", slot))
			var col: Color
			match idx % 6:
				0: col = Color(1.0, 0.4, 0.6) # Clover / Magenta bloom
				1: col = Color(0.95, 0.85, 0.2) # Goldenrod yellow
				2: col = Color(0.55, 0.35, 0.85) # Violet bellflower
				3: col = Color(0.92, 0.94, 0.95) # Alpine Edelweiss
				4: col = Color(0.20, 0.52, 0.18) # Deep woodland shrub
				_: col = Color(0.32, 0.65, 0.22) # Meadow grass tuft
			mm.set_instance_color(slot, col)
			placed_count += 1
		else:
			t.origin = Vector3(0, -1000, 0) # park unused instances underground
			t.basis = t.basis.scaled(Vector3(0.001, 0.001, 0.001))
			mm.set_instance_transform(slot, t)
			mm.set_instance_color(slot, Color(0, 0, 0, 0))

	multimesh_instance.multimesh = mm
	print("[VegetationRenderer] Instances: placed=%d/%d backend=%s." % [placed_count, mm.instance_count, str(_using_backend_data)])

## Consume canonical backend vegetation state.
## Expected: Array of Dictionaries with at least "pos" (Vector3); optional
## "scale" (float) and "kind" ("flower"/"shrub"). Returns placed count.
func apply_backend_instances(items: Array) -> int:
	_using_backend_data = true
	var normalized: Array = []
	for i in range(items.size()):
		var e: Dictionary = items[i]
		if not e.has("pos"):
			continue
		normalized.append({
			"pos": e["pos"],
			"scale": float(e.get("scale", 1.0)),
			"index": i if str(e.get("kind", "shrub")) != "flower" else 0,
		})
	apply_positions(normalized)
	return placed_count

func is_using_backend_data() -> bool:
	return _using_backend_data

static func sample_height(x: float, z: float) -> float:
	# Same visual approximation constants as TerrainRenderer.sample_height().
	# Duplicated (not cross-referenced) so each renderer parses standalone
	# without depending on global class resolution order.
	var h1: float = sin(x * 0.05) * cos(z * 0.05) * 4.0
	var h2: float = sin(x * 0.12 + 1.2) * cos(z * 0.12 + 0.8) * 1.5
	return h1 + h2

func get_placed_count() -> int:
	return placed_count
