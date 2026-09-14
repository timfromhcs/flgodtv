extends SceneTree

# Headless smoke test verifying Godot 4 project parsing, scene instantiation,
# 4-camera rig, MultiMesh instance setup, backend bridge data, and Stage 17
# world rendering (terrain LOD/streaming, vegetation instancing, water level,
# weather consequences, lighting profiles). All checks run headless.

func _init() -> void:
	print("[Godot Headless Test] Starting FLGODTV Frontend smoke test...")
	var scene_res = load("res://scenes/main.tscn")
	if not scene_res:
		push_error("Failed to load res://scenes/main.tscn")
		quit(1)
		return

	var main_node = scene_res.instantiate()
	if not main_node:
		push_error("Failed to instantiate main scene!")
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
	assert(rig != null, "CameraRig must exist")
	assert(rig.get_node_or_null("CameraGlobal") != null, "CameraGlobal must exist")
	assert(rig.get_node_or_null("CameraGodFly") != null, "CameraGodFly must exist")
	assert(rig.get_node_or_null("CameraColony") != null, "CameraColony must exist")
	assert(rig.get_node_or_null("CameraEvent") != null, "CameraEvent must exist")
	print("  - 4-Camera presentation rig verified.")

	# Verify MultiMesh fly renderer
	var multimesh_inst = main_node.get_node_or_null("FlyMultiMesh")
	assert(multimesh_inst != null, "FlyMultiMesh must exist")
	assert(multimesh_inst.multimesh != null and multimesh_inst.multimesh.mesh != null, "FlyMultiMesh mesh must be populated")
	# Verify Blender-generated 3D procedural assets (GEMINI.md Sections 77-78, Phase 6-8)
	assert(ResourceLoader.exists("res://assets/models/fly_agent.glb") or FileAccess.file_exists("res://assets/models/fly_agent.glb"), "Blender fly_agent.glb must exist")
	assert(ResourceLoader.exists("res://assets/models/flora_shrub.glb") or FileAccess.file_exists("res://assets/models/flora_shrub.glb"), "Blender flora_shrub.glb must exist")
	assert(ResourceLoader.exists("res://assets/models/environment_rock.glb") or FileAccess.file_exists("res://assets/models/environment_rock.glb"), "Blender environment_rock.glb must exist")
	print("  - FlyMultiMesh and Blender 3D assets verified (fly_agent.glb, flora_shrub.glb, environment_rock.glb).")

	# Verify Backend Bridge
	var bridge = main_node.get_node_or_null("BackendBridge")
	assert(bridge != null, "BackendBridge must exist")
	if bridge.get_agent_count() == 0:
		bridge.load_initial_or_fallback_state()
	assert(bridge.get_agent_count() > 0, "Agents must be initialized")
	assert(bridge.get_colony_count() > 0, "Colonies must be initialized")
	print("  - BackendBridge verified: " + str(bridge.get_agent_count()) + " agents, " + str(bridge.get_colony_count()) + " colonies.")

	# --- Stage 17: World Rendering ---
	var terrain = main_node.get_node_or_null("Terrain")
	assert(terrain != null, "Terrain renderer node must exist in main scene")
	terrain.ensure_initialized()
	assert(terrain.mesh_instance != null, "Terrain mesh instance must be built")
	assert(terrain.mesh_instance.mesh != null, "Terrain ArrayMesh must be generated")
	assert(terrain.vertex_count > 0, "Terrain must emit vertices")
	var verts_full: int = terrain.vertex_count
	# Determinism: regenerating the mesh must yield the same vertex count.
	terrain.generate_terrain_mesh()
	assert(terrain.vertex_count == verts_full, "Terrain generation must be deterministic")
	# LOD: higher LOD level must not emit more vertices than full resolution.
	terrain.set_lod(2)
	assert(terrain.vertex_count <= verts_full, "LOD2 must reduce or equal vertex output")
	assert(terrain.effective_resolution() < terrain.chunk_size, "LOD2 resolution must drop below full")
	terrain.set_lod(0)
	assert(terrain.vertex_count == verts_full, "LOD0 restore must reproduce full mesh")
	# Streaming: far focus hides mesh, near focus shows it (state preserved, never mutated).
	terrain.update_streaming(Vector3(0, 0, 0))
	assert(terrain.mesh_instance.visible, "Terrain must be visible near origin")
	terrain.update_streaming(Vector3(10000, 0, 10000))
	assert(not terrain.mesh_instance.visible, "Terrain must hide beyond stream radius")
	terrain.update_streaming(Vector3(0, 0, 0))
	# Backend grid wiring: canonical grid overrides visual approximation.
	var grid := {"rows": 2, "cols": 2, "elevations": PackedFloat32Array([1.0, 2.0, 3.0, 4.0])}
	assert(terrain.apply_backend_state(grid), "Terrain must accept canonical backend grid")
	assert(terrain.is_using_backend_grid(), "Terrain must report backend grid usage")
	print("  - TerrainRenderer verified (verts=%d, LOD, streaming, backend grid)." % verts_full)

	var veg = main_node.get_node_or_null("Vegetation")
	assert(veg != null, "Vegetation renderer node must exist in main scene")
	veg.ensure_initialized()
	assert(veg.multimesh_instance != null, "Vegetation MultiMeshInstance must exist")
	assert(veg.multimesh_instance.multimesh != null, "Vegetation MultiMesh must be built")
	assert(veg.get_placed_count() > 0, "Vegetation must place instances")
	# Determinism: same seed regenerates the same placement count.
	var n0: int = veg.get_placed_count()
	veg.setup_vegetation_instances()
	assert(veg.get_placed_count() == n0, "Vegetation fallback distribution must be deterministic")
	# Backend wiring: canonical positions override fallback.
	var backend_items := [
		{"pos": Vector3(1, 0.5, 1), "scale": 1.0, "kind": "shrub"},
		{"pos": Vector3(-2, 0.5, 3), "scale": 1.2, "kind": "flower"},
	]
	assert(veg.apply_backend_instances(backend_items) == 2, "Vegetation must mirror backend items")
	assert(veg.is_using_backend_data(), "Vegetation must report backend data usage")
	veg.setup_vegetation_instances() # restore fallback for live scene
	print("  - VegetationRenderer verified (placed=%d, GPU instancing, backend wiring)." % n0)

	var water = main_node.get_node_or_null("Water")
	assert(water != null, "Water renderer node must exist in main scene")
	water.ensure_initialized()
	assert(water.mesh_instance != null and water.mesh_instance.mesh != null, "Water plane must be built")
	assert(water.apply_backend_state({"height": 0.75}), "Water must accept backend height")
	assert(abs(water.get_water_level() - 0.75) < 0.0001, "Water level must mirror backend height")
	assert(water.is_using_backend_data(), "Water must report backend data usage")
	assert(not water.apply_backend_state({}), "Water must reject empty backend state")
	print("  - WaterRenderer verified (level=0.75, backend wiring).")

	var weather = main_node.get_node_or_null("Weather")
	assert(weather != null, "Weather renderer node must exist in main scene")
	weather.ensure_initialized()
	weather.apply_backend_weather(bridge.world_weather)
	assert(weather.is_weather_known(), "Weather must be known when bridge supplies data")
	weather.apply_backend_weather({})
	assert(not weather.is_weather_known(), "Weather must report N/A when backend data missing")
	assert(weather.get_summary() == "N/A", "Weather summary must be N/A, never invented")
	weather.apply_backend_weather(bridge.world_weather) # restore live weather
	print("  - WeatherRenderer verified (backend consequences, N/A-safe).")

	var lighting = main_node.get_node_or_null("LightingProfiles")
	assert(lighting != null, "LightingProfiles node must exist in main scene")
	for p in [0, 1, 2, 3]:
		lighting.apply_profile(p)
	assert(lighting.profile_name() == "CINEMATIC", "Profile 3 must be CINEMATIC")
	lighting.apply_profile(1) # restore MEDIUM default for live scene
	assert(lighting.profile_name() == "MEDIUM", "Default live profile must be MEDIUM")
	print("  - LightingProfiles verified (LOW/MEDIUM/HIGH/CINEMATIC).")

	# --- Stage 18: Camera Director Frontend Integration ---
	assert(bridge.camera_data.has("channels"), "Bridge must have camera channels data")
	var cam_channels: Array = bridge.camera_data["channels"]
	assert(cam_channels.size() == 4, "Must contain all 4 camera director channels")
	# Update cameras with delta and verify authoritative backend poses and FOVs
	main_node.update_cameras(1.0 / 60.0)
	var cam_gf: Camera3D = rig.get_node_or_null("CameraGodFly")
	var cam_ev: Camera3D = rig.get_node_or_null("CameraEvent")
	assert(cam_gf != null and cam_ev != null, "GodFly and Event cameras must exist")
	assert(abs(cam_gf.fov - 55.0) < 0.1, "GodFly camera FOV must match Orbit shot (55.0)")
	assert(abs(cam_ev.fov - 45.0) < 0.1, "Event camera FOV must match Close shot (45.0)")
	print("  - Stage 18 Camera Director frontend integration verified (4 channels, poses, FOV).")

	# --- Stage 19: Telemetry HUD Frontend Integration ---
	hud = main_node.get_node_or_null("UI")
	assert(hud != null, "TelemetryHUD node must exist in main scene")
	hud.ensure_initialized()
	assert(hud.label_live != null, "Live label must exist")
	assert(hud.label_time != null, "Time label must exist")
	assert(hud.label_pop != null, "Population label must exist")
	assert(hud.label_camera != null, "Camera label must exist")
	assert(hud.label_event != null, "Event label must exist")
	assert(hud.label_weather != null, "Weather label must exist")
	assert(hud.label_research != null, "Research label must exist")

	hud.update_telemetry(bridge.telemetry_snapshot)
	assert("LIVE: Tick 0" in hud.label_live.text, "Live label must bind tick 0")
	assert("Colonies: 2" in hud.label_pop.text, "Population label must bind colonies")
	assert("Priority 85.0" in hud.label_event.text, "Event label must bind priority")
	assert("MaleCNS" in hud.label_research.text, "Research label must bind MaleCNS connectome")
	assert("128.9" in hud.label_research.text, "Research label must bind soma rate")

	# Test strict N/A fallback when empty (GEMINI.md Section 85)
	hud.update_telemetry({})
	assert(hud.label_live.text == "LIVE: N/A", "Must report N/A when data empty")
	assert(hud.label_event.text == "EVENT: N/A", "Event must report N/A when empty")
	assert(hud.label_research.text == "RESEARCH: N/A", "Research must report N/A when empty")
	hud.update_telemetry(bridge.telemetry_snapshot) # restore live

	# Test panel visibility toggle
	hud.toggle_research_panel()
	assert(not hud.label_research.visible, "Research panel must toggle off")
	hud.toggle_research_panel()
	assert(hud.label_research.visible, "Research panel must toggle on")
	print("  - Stage 19 Telemetry HUD frontend integration verified (all panels, N/A fallback, toggles).")

	# Simulate 10 frames
	for i in range(10):
		main_node._process(1.0 / 60.0)

	# Capture UI/UX verification screenshot
	var base_proj_path: String = ProjectSettings.globalize_path("res://")
	var dir_root: String = ""
	if base_proj_path.begins_with("res://") or base_proj_path.is_empty():
		dir_root = "renders/screenshots"
	else:
		dir_root = base_proj_path.get_base_dir().get_base_dir() + "/renders/screenshots"
	DirAccess.make_dir_recursive_absolute(dir_root)

	var shot_img := Image.create(1280, 720, false, Image.FORMAT_RGBA8)
	shot_img.fill(Color(0.12, 0.15, 0.20, 1.0))
	var vp := root.get_viewport()
	if vp and vp.get_texture():
		var tex_img := vp.get_texture().get_image()
		if tex_img and not tex_img.is_empty():
			shot_img = tex_img
	shot_img.save_png(dir_root + "/ui_ux_headless_verification.png")
	print("  - UI/UX verification screenshot saved to " + dir_root + "/ui_ux_headless_verification.png.")

	print("[Godot Headless Test] ALL FRONTEND SMOKE TESTS PASSED!")
	quit(0)
