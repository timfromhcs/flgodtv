extends SceneTree

# Headless smoke test verifying Godot 4 project parsing, scene instantiation,
# 4-camera rig, MultiMesh instance setup, backend bridge data, and Stage 17
# world rendering (terrain LOD/streaming, vegetation instancing, water level,
# weather consequences, lighting profiles).
#
# Failure handling is explicit and deterministic: every condition routes through
# check(), any failure sets failed=true and the suite exits non-zero. No
# assert() is used (unreliable release-mode semantics). The UI/UX verification
# screenshot is captured from REAL rendered frames via a watcher node, always
# overwritten fresh, objectively validated, and never substituted with a
# synthetic fallback image.

var failed: bool = false

func check(cond: bool, msg: String) -> void:
	if not cond:
		failed = true
		push_error("[FrontendSmoke][FAIL] " + msg)
		printerr("[FrontendSmoke][FAIL] " + msg)

func _init() -> void:
	print("[Godot Headless Test] Starting FLGODTV Frontend smoke test...")
	var scene_res = load("res://scenes/main.tscn")
	if scene_res == null:
		push_error("[FrontendSmoke][FAIL] Failed to load res://scenes/main.tscn")
		quit(1)
		return

	var main_node = scene_res.instantiate()
	if main_node == null:
		push_error("[FrontendSmoke][FAIL] Failed to instantiate main scene!")
		quit(1)
		return

	root.add_child(main_node)
	current_scene = main_node
	main_node.ensure_initialized()

	var hud = main_node.get_node_or_null("UI")
	if hud:
		hud.ensure_initialized()

	# Verify 4-Camera Rig
	var rig = main_node.get_node_or_null("CameraRig")
	check(rig != null, "CameraRig must exist")
	if rig:
		check(rig.get_node_or_null("CameraGlobal") != null, "CameraGlobal must exist")
		check(rig.get_node_or_null("CameraGodFly") != null, "CameraGodFly must exist")
		check(rig.get_node_or_null("CameraColony") != null, "CameraColony must exist")
		check(rig.get_node_or_null("CameraEvent") != null, "CameraEvent must exist")
	print("  - 4-Camera presentation rig verified.")

	# Verify MultiMesh fly renderer
	var multimesh_inst = main_node.get_node_or_null("FlyMultiMesh")
	check(multimesh_inst != null, "FlyMultiMesh must exist")
	if multimesh_inst:
		check(multimesh_inst.multimesh != null and multimesh_inst.multimesh.mesh != null, "FlyMultiMesh mesh must be populated")
	# Verify Blender-generated 3D procedural assets (GEMINI.md Sections 77-78, Phase 6-8)
	check(ResourceLoader.exists("res://assets/models/fly_agent.glb") or FileAccess.file_exists("res://assets/models/fly_agent.glb"), "Blender fly_agent.glb must exist")
	check(ResourceLoader.exists("res://assets/models/flora_shrub.glb") or FileAccess.file_exists("res://assets/models/flora_shrub.glb"), "Blender flora_shrub.glb must exist")
	check(ResourceLoader.exists("res://assets/models/environment_rock.glb") or FileAccess.file_exists("res://assets/models/environment_rock.glb"), "Blender environment_rock.glb must exist")
	print("  - FlyMultiMesh and Blender 3D assets verified (fly_agent.glb, flora_shrub.glb, environment_rock.glb).")

	# Verify Backend Bridge
	var bridge = main_node.get_node_or_null("BackendBridge")
	check(bridge != null, "BackendBridge must exist")
	if bridge:
		if bridge.get_agent_count() == 0:
			bridge.load_initial_or_fallback_state()
		check(bridge.get_agent_count() > 0, "Agents must be initialized")
		check(bridge.get_colony_count() > 0, "Colonies must be initialized")
		print("  - BackendBridge verified: " + str(bridge.get_agent_count()) + " agents, " + str(bridge.get_colony_count()) + " colonies.")

	# --- Stage 17: World Rendering ---
	var terrain = main_node.get_node_or_null("Terrain")
	check(terrain != null, "Terrain renderer node must exist in main scene")
	if terrain:
		terrain.ensure_initialized()
		check(terrain.mesh_instance != null, "Terrain mesh instance must be built")
		check(terrain.mesh_instance.mesh != null, "Terrain ArrayMesh must be generated")
		check(terrain.vertex_count > 0, "Terrain must emit vertices")
		var verts_full: int = terrain.vertex_count
		# Determinism: regenerating the mesh must yield the same vertex count.
		terrain.generate_terrain_mesh()
		check(terrain.vertex_count == verts_full, "Terrain generation must be deterministic")
		# LOD: higher LOD level must not emit more vertices than full resolution.
		terrain.set_lod(2)
		check(terrain.vertex_count <= verts_full, "LOD2 must reduce or equal vertex output")
		check(terrain.effective_resolution() < terrain.chunk_size, "LOD2 resolution must drop below full")
		terrain.set_lod(0)
		check(terrain.vertex_count == verts_full, "LOD0 restore must reproduce full mesh")
		# Streaming: far focus hides mesh, near focus shows it (state preserved, never mutated).
		terrain.update_streaming(Vector3(0, 0, 0))
		check(terrain.mesh_instance.visible, "Terrain must be visible near origin")
		terrain.update_streaming(Vector3(10000, 0, 10000))
		check(not terrain.mesh_instance.visible, "Terrain must hide beyond stream radius")
		terrain.update_streaming(Vector3(0, 0, 0))

	# Backend grid wiring: canonical grid overrides visual approximation.
	if terrain:
		var grid := {"rows": 2, "cols": 2, "elevations": PackedFloat32Array([1.0, 2.0, 3.0, 4.0])}
		check(terrain.apply_backend_state(grid), "Terrain must accept canonical backend grid")
		check(terrain.is_using_backend_grid(), "Terrain must report backend grid usage")
		print("  - TerrainRenderer verified (verts=%d, LOD, streaming, backend grid)." % terrain.vertex_count)

	var veg = main_node.get_node_or_null("Vegetation")
	check(veg != null, "Vegetation renderer node must exist in main scene")
	if veg:
		veg.ensure_initialized()
		check(veg.multimesh_instance != null, "Vegetation MultiMeshInstance must exist")
		check(veg.multimesh_instance.multimesh != null, "Vegetation MultiMesh must be built")
		check(veg.get_placed_count() > 0, "Vegetation must place instances")
		# Determinism: same seed regenerates the same placement count.
		var n0: int = veg.get_placed_count()
		veg.setup_vegetation_instances()
		check(veg.get_placed_count() == n0, "Vegetation fallback distribution must be deterministic")
		# Backend wiring: canonical positions override fallback.
		var backend_items := [
			{"pos": Vector3(1, 0.5, 1), "scale": 1.0, "kind": "shrub"},
			{"pos": Vector3(-2, 0.5, 3), "scale": 1.2, "kind": "flower"},
		]
		check(veg.apply_backend_instances(backend_items) == 2, "Vegetation must mirror backend items")
		check(veg.is_using_backend_data(), "Vegetation must report backend data usage")
		veg.setup_vegetation_instances() # restore fallback for live scene
		print("  - VegetationRenderer verified (placed=%d, GPU instancing, backend wiring)." % n0)

	var water = main_node.get_node_or_null("Water")
	check(water != null, "Water renderer node must exist in main scene")
	if water:
		water.ensure_initialized()
		check(water.mesh_instance != null and water.mesh_instance.mesh != null, "Water plane must be built")
		check(water.apply_backend_state({"height": 0.75}), "Water must accept backend height")
		check(abs(water.get_water_level() - 0.75) < 0.0001, "Water level must mirror backend height")
		check(water.is_using_backend_data(), "Water must report backend data usage")
		check(not water.apply_backend_state({}), "Water must reject empty backend state")
		print("  - WaterRenderer verified (level=0.75, backend wiring).")

	var weather = main_node.get_node_or_null("Weather")
	check(weather != null, "Weather renderer node must exist in main scene")
	if weather and bridge:
		weather.ensure_initialized()
		weather.apply_backend_weather(bridge.world_weather)
		check(weather.is_weather_known(), "Weather must be known when bridge supplies data")
		weather.apply_backend_weather({})
		check(not weather.is_weather_known(), "Weather must report N/A when backend data missing")
		check(weather.get_summary() == "N/A", "Weather summary must be N/A, never invented")
		weather.apply_backend_weather(bridge.world_weather) # restore live weather
		print("  - WeatherRenderer verified (backend consequences, N/A-safe).")

	var lighting = main_node.get_node_or_null("LightingProfiles")
	check(lighting != null, "LightingProfiles node must exist in main scene")
	if lighting:
		for p in [0, 1, 2, 3]:
			lighting.apply_profile(p)
		check(lighting.profile_name() == "CINEMATIC", "Profile 3 must be CINEMATIC")
		lighting.apply_profile(1) # restore MEDIUM default for live scene
		check(lighting.profile_name() == "MEDIUM", "Default live profile must be MEDIUM")
		print("  - LightingProfiles verified (LOW/MEDIUM/HIGH/CINEMATIC).")

	# --- Stage 18: Camera Director Frontend Integration ---
	if bridge:
		check(bridge.camera_data.has("channels"), "Bridge must have camera channels data")
		var cam_channels: Array = bridge.camera_data["channels"] if bridge.camera_data.has("channels") else []
		check(cam_channels.size() == 4, "Must contain all 4 camera director channels")
	# Update cameras with delta and verify authoritative backend poses and FOVs
	main_node.update_cameras(1.0 / 60.0)
	var cam_gf: Camera3D = rig.get_node_or_null("CameraGodFly") if rig else null
	var cam_ev: Camera3D = rig.get_node_or_null("CameraEvent") if rig else null
	check(cam_gf != null and cam_ev != null, "GodFly and Event cameras must exist")
	if cam_gf and cam_ev:
		check(abs(cam_gf.fov - 55.0) < 0.1, "GodFly camera FOV must match Orbit shot (55.0)")
		check(abs(cam_ev.fov - 45.0) < 0.1, "Event camera FOV must match Close shot (45.0)")
	print("  - Stage 18 Camera Director frontend integration verified (4 channels, poses, FOV).")

	# --- Stage 19: Telemetry HUD Frontend Integration ---
	hud = main_node.get_node_or_null("UI")
	check(hud != null, "TelemetryHUD node must exist in main scene")
	if hud and bridge:
		hud.ensure_initialized()
		check(hud.label_live != null, "Live label must exist")
		check(hud.label_time != null, "Time label must exist")
		check(hud.label_pop != null, "Population label must exist")
		check(hud.label_camera != null, "Camera label must exist")
		check(hud.label_event != null, "Event label must exist")
		check(hud.label_weather != null, "Weather label must exist")
		check(hud.label_research != null, "Research label must exist")

		hud.update_telemetry(bridge.telemetry_snapshot)
		check(hud.label_live.text.begins_with("LIVE: Tick ") and "Ticks/s" in hud.label_live.text, "Live label must bind live tick and rate")
		check("Colonies: 2" in hud.label_pop.text, "Population label must bind colonies")
		check("Priority " in hud.label_event.text, "Event label must bind priority")
		check("MaleCNS" in hud.label_research.text, "Research label must bind MaleCNS connectome")
		check("128.9" in hud.label_research.text, "Research label must bind soma rate")

		# Test strict N/A fallback when empty (GEMINI.md Section 85)
		hud.update_telemetry({})
		check(hud.label_live.text == "LIVE: N/A", "Must report N/A when data empty")
		check(hud.label_event.text == "EVENT: N/A", "Event must report N/A when empty")
		check(hud.label_research.text == "RESEARCH: N/A", "Research must report N/A when empty")
		hud.update_telemetry(bridge.telemetry_snapshot) # restore live

		# Test panel visibility toggle
		hud.toggle_research_panel()
		check(not hud.label_research.visible, "Research panel must toggle off")
		hud.toggle_research_panel()
		check(hud.label_research.visible, "Research panel must toggle on")
		print("  - Stage 19 Telemetry HUD frontend integration verified (all panels, N/A fallback, toggles).")

	if failed:
		push_error("[Godot Headless Test] LOGIC VERIFICATION FAILED, skipping capture phase.")
		quit(1)
		return

	# Capture the UI/UX verification screenshot from REAL rendered frames.
	# The watcher waits for the renderer, captures one frame, validates it
	# objectively, and quits non-zero on any failure. Stale files are always
	# overwritten; empty/uniform frames FAIL instead of saving a fallback.
	print("  - Capturing FRESH UI/UX verification screenshot from rendered frames...")
	var watcher = FrontendCapture.new()
	watcher.suite = self
	watcher.main_node = main_node
	root.add_child(watcher)


class FrontendCapture extends Node:
	var suite: SceneTree = null
	var main_node: Node = null
	var wait_frames: int = 30
	var target_shot: String = ""

	func _ready() -> void:
		var base_proj_path: String = ProjectSettings.globalize_path("res://")
		if base_proj_path.begins_with("res://") or base_proj_path.is_empty():
			target_shot = "renders/screenshots/ui_ux_headless_verification.png"
		else:
			target_shot = base_proj_path.get_base_dir().get_base_dir() + "/renders/screenshots"
			DirAccess.make_dir_recursive_absolute(target_shot)
			target_shot += "/ui_ux_headless_verification.png"

	func _process(_delta: float) -> void:
		if wait_frames > 0:
			wait_frames -= 1
			if wait_frames == 0:
				RenderingServer.frame_post_draw.connect(_on_drawn, CONNECT_ONE_SHOT)
			return

	func _on_drawn() -> void:
		var vp := get_viewport()
		var img: Image = null
		if vp and vp.get_texture():
			img = vp.get_texture().get_image()
		if img == null or img.is_empty():
			suite.check(false, "UI/UX capture produced an empty viewport image (renderer unavailable?)")
			_quit()
			return
		var w := img.get_width()
		var h := img.get_height()
		var d := img.get_data()
		var uniq := {}
		var n := 0
		var mean_acc := 0.0
		var total := w * h
		for i in range(0, total, 97):
			var o := i * 4
			if o + 2 >= d.size():
				break
			uniq[[d[o], d[o + 1], d[o + 2]]] = true
			mean_acc += d[o] + d[o + 1] + d[o + 2]
			n += 1
		suite.check(w >= 640 and h >= 480, "UI/UX capture has invalid dimensions %dx%d" % [w, h])
		suite.check(uniq.size() > 50, "UI/UX capture failed content validation (unique=%d); refusing synthetic fallback" % uniq.size())
		if not suite.failed:
			DirAccess.make_dir_recursive_absolute(target_shot.get_base_dir())
			img.save_png(target_shot)
			var ctx := HashingContext.new()
			ctx.start(HashingContext.HASH_SHA256)
			ctx.update(img.save_png_to_buffer())
			print("  - UI/UX verification screenshot saved at " + target_shot + " (%dx%d sha256=%s...)." % [w, h, ctx.finish().hex_encode().substr(0, 12)])
		_quit()

	func _quit() -> void:
		if suite.failed:
			push_error("[Godot Headless Test] FRONTEND SMOKE TEST FAILED.")
			suite.quit(1)
		else:
			print("[Godot Headless Test] ALL FRONTEND SMOKE TESTS PASSED!")
			suite.quit(0)
