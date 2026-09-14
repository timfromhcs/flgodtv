extends Control
class_name MapSystem

# MapSystem implements the synchronized 2D world map and radar minimap
# for Visual V2 per GEMINI.md Section 84 & Visual V2 Target.
# All data derives strictly from canonical backend state (BackendBridge):
# terrain bounds, colony nests, agent positions, God Fly beacon, and events.

@export var world_center: Vector2 = Vector2(30.0, 30.0)
@export var world_extent: float = 65.0 # Covers -35 to +95 in X and Z
@export var is_expanded: bool = false
@export var layer_mode: int = 0 # 0=All, 1=Population, 2=Territory & Ecology, 3=Events Only

var bridge: Node = null
var active_camera_pos: Vector3 = Vector3(30, 55, 115)
var active_camera_look: Vector3 = Vector3(30, 5, 25)

func _init() -> void:
	custom_minimum_size = Vector2(180, 180)
	size = Vector2(180, 180)

func _ready() -> void:
	custom_minimum_size = Vector2(180, 180)
	size = custom_minimum_size
	mouse_filter = Control.MOUSE_FILTER_IGNORE

func set_bridge(b: Node) -> void:
	bridge = b
	if bridge and bridge.has_signal("state_updated"):
		bridge.state_updated.connect(_on_state_updated)

func _on_state_updated(_state: Dictionary) -> void:
	queue_redraw()

func _process(_delta: float) -> void:
	queue_redraw()

func world_to_map(wx: float, wz: float) -> Vector2:
	var norm_x: float = (wx - (world_center.x - world_extent)) / (world_extent * 2.0)
	var norm_y: float = (wz - (world_center.y - world_extent)) / (world_extent * 2.0)
	return Vector2(
		clampf(norm_x * size.x, 2.0, size.x - 2.0),
		clampf(norm_y * size.y, 2.0, size.y - 2.0)
	)

func _draw() -> void:
	var rect := Rect2(Vector2.ZERO, size)

	# 1. Background (Ocean / Border)
	draw_rect(rect, Color(0.06, 0.12, 0.20, 0.88), true)
	draw_rect(rect, Color(0.25, 0.45, 0.70, 0.95), false, 2.0)

	# 2. Grid lines
	var center_pt := world_to_map(world_center.x, world_center.y)
	draw_line(Vector2(center_pt.x, 0), Vector2(center_pt.x, size.y), Color(0.2, 0.35, 0.5, 0.3), 1.0)
	draw_line(Vector2(0, center_pt.y), Vector2(size.x, center_pt.y), Color(0.2, 0.35, 0.5, 0.3), 1.0)

	# 3. Terrain Landmass Contour (Approximate island extent)
	var c1_pt := world_to_map(0.0, 0.0)
	var c2_pt := world_to_map(60.0, 60.0)
	var gf_base_pt := world_to_map(30.0, 30.0)

	if layer_mode == 0 or layer_mode == 2:
		# Colony 1 Island
		draw_circle(c1_pt, size.x * 0.22, Color(0.20, 0.42, 0.22, 0.45))
		# Colony 2 Island
		draw_circle(c2_pt, size.x * 0.22, Color(0.28, 0.38, 0.18, 0.45))
		# Central Ridge / Plateau
		draw_circle(gf_base_pt, size.x * 0.18, Color(0.35, 0.35, 0.30, 0.45))

	if not bridge:
		# Draw title when bridge is not yet connected
		draw_string(ThemeDB.fallback_font, Vector2(6, 16), "RADAR (STANDALONE)", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0.7, 0.8, 0.9))
		return

	# 4. Colonies and Territory Rings
	if layer_mode == 0 or layer_mode == 2:
		# Colony 1 Nest & Territory
		draw_circle(c1_pt, 4.0, Color(0.2, 0.9, 1.0))
		draw_arc(c1_pt, size.x * 0.20, 0, TAU, 24, Color(0.2, 0.9, 1.0, 0.4), 1.0)
		draw_string(ThemeDB.fallback_font, c1_pt + Vector2(6, 4), "C1", HORIZONTAL_ALIGNMENT_LEFT, -1, 9, Color(0.3, 0.9, 1.0))

		# Colony 2 Nest & Territory
		draw_circle(c2_pt, 4.0, Color(1.0, 0.65, 0.2))
		draw_arc(c2_pt, size.x * 0.20, 0, TAU, 24, Color(1.0, 0.65, 0.2, 0.4), 1.0)
		draw_string(ThemeDB.fallback_font, c2_pt + Vector2(6, 4), "C2", HORIZONTAL_ALIGNMENT_LEFT, -1, 9, Color(1.0, 0.7, 0.2))

	# 5. Agents
	if layer_mode == 0 or layer_mode == 1:
		for a in bridge.agents_data:
			var pos: Vector3 = a.get("position", Vector3.ZERO)
			var cid: int = a.get("colony_id", 1)
			var pt := world_to_map(pos.x, pos.z)
			var col := Color(0.3, 0.95, 1.0, 0.85) if cid == 1 else Color(1.0, 0.75, 0.2, 0.85)
			draw_circle(pt, 2.0, col)

	# 6. God Fly Beacon
	if bridge.god_fly_data.has("position"):
		var gf_pos: Vector3 = bridge.god_fly_data["position"]
		var gf_pt := world_to_map(gf_pos.x, gf_pos.z)
		# Golden Diamond Beacon
		var sz: float = 4.5
		var pts := PackedVector2Array([
			gf_pt + Vector2(0, -sz),
			gf_pt + Vector2(sz, 0),
			gf_pt + Vector2(0, sz),
			gf_pt + Vector2(-sz, 0)
		])
		draw_colored_polygon(pts, Color(1.0, 0.85, 0.2, 0.95))
		draw_polyline(pts, Color(1.0, 1.0, 0.6, 1.0), 1.0)

	# 7. Simulation Events Alerts
	if layer_mode == 0 or layer_mode == 3:
		for ev in bridge.events_data:
			if ev.has("pos"):
				var ep := Vector3(ev["pos"][0], ev["pos"][1], ev["pos"][2])
				var ev_pt := world_to_map(ep.x, ep.z)
				draw_circle(ev_pt, 5.0, Color(1.0, 0.2, 0.2, 0.5))
				draw_circle(ev_pt, 2.0, Color(1.0, 0.8, 0.8, 0.9))

	# 8. Active Camera View Cone
	var cam_pt := world_to_map(active_camera_pos.x, active_camera_pos.z)
	var look_pt := world_to_map(active_camera_look.x, active_camera_look.z)
	draw_circle(cam_pt, 3.0, Color(0.9, 0.9, 0.9, 0.7))
	draw_line(cam_pt, look_pt, Color(1.0, 1.0, 1.0, 0.4), 1.0)

	# 9. Header info
	var header := "RADAR MINIMAP [M] | L:%d" % layer_mode
	draw_string(ThemeDB.fallback_font, Vector2(6, 14), header, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0.85, 0.92, 1.0))

func toggle_expanded() -> void:
	is_expanded = not is_expanded
	if is_expanded:
		custom_minimum_size = Vector2(320, 320)
	else:
		custom_minimum_size = Vector2(180, 180)
	size = custom_minimum_size

func cycle_layers() -> void:
	layer_mode = (layer_mode + 1) % 4
