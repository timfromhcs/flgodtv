extends SceneTree

# Diagnostic script to run main.tscn with real rendering, capture frame,
# and inspect camera, lighting, terrain, and viewport state.

class FrameWatcher extends Node:
	var frames_left: int = 50
	var captured_solo: bool = false

	func _process(_delta: float) -> void:
		frames_left -= 1
		if frames_left == 25 and not captured_solo:
			captured_solo = true
			_capture_screenshot("diagnostic_screen.png")
			# Switch to Quad-View
			var main_node = get_tree().root.get_node_or_null("FLGODTV")
			if main_node and main_node.has_method("set_view_mode"):
				main_node.set_view_mode(0)
		elif frames_left <= 0:
			_capture_screenshot("quadview_presentation_proof.png")
			get_tree().quit(0)

	func _capture_screenshot(filename: String) -> void:
		var vp = get_viewport()
		var tex = vp.get_texture()
		if tex:
			var img = tex.get_image()
			if img and not img.is_empty():
				var out_path = ProjectSettings.globalize_path("res://../renders/screenshots/" + filename)
				DirAccess.make_dir_recursive_absolute("renders/screenshots")
				img.save_png(out_path)
				print("[RenderCapture] Saved screenshot to: ", out_path, " (", img.get_width(), "x", img.get_height(), ")")

	func _capture_and_diagnose() -> void:
		print("[RenderCapture] Diagnosing scene state at frame 30...")
		var root_node = get_tree().root
		var vp = root_node.get_viewport()
		print("  - Viewport size: ", vp.size)
		var cam = vp.get_camera_3d()
		print("  - Active Camera3D: ", cam.name if cam else "NONE")
		if cam:
			print("    * Camera position: ", cam.global_position)
			print("    * Camera rotation: ", cam.global_rotation_degrees)
			print("    * Camera FOV: ", cam.fov)
			print("    * Camera near/far: ", cam.near, " / ", cam.far)
			print("    * Camera current: ", cam.current)

		var main_node = root_node.get_node_or_null("FLGODTV")
		if main_node:
			var env_node = main_node.get_node_or_null("WorldEnvironment")
			if env_node and env_node.environment:
				print("  - WorldEnvironment: background_mode=", env_node.environment.background_mode, " ambient_light_source=", env_node.environment.ambient_light_source)
			else:
				print("  - WorldEnvironment: MISSING or NO ENVIRONMENT RESOURCE!")

			var light_node = main_node.get_node_or_null("DirectionalLight3D")
			if light_node:
				print("  - DirectionalLight3D: visible=", light_node.visible, " energy=", light_node.light_energy, " pos=", light_node.global_position)
			else:
				print("  - DirectionalLight3D: MISSING!")

			var terrain = main_node.get_node_or_null("Terrain")
			if terrain:
				print("  - Terrain node: visible=", terrain.visible, " child_count=", terrain.get_child_count())
				if terrain.mesh_instance:
					print("    * TerrainMesh: visible=", terrain.mesh_instance.visible, " verts=", terrain.vertex_count, " aabb=", terrain.mesh_instance.get_aabb())
				else:
					print("    * TerrainMesh: NULL!")

			var mm_node = main_node.get_node_or_null("FlyMultiMesh")
			if mm_node:
				print("  - FlyMultiMesh: visible=", mm_node.visible, " mm=", mm_node.multimesh != null)
				if mm_node.multimesh:
					print("    * MultiMesh instance_count=", mm_node.multimesh.instance_count, " mesh=", mm_node.multimesh.mesh != null)

			var gf_node = main_node.get_node_or_null("GodFlyMesh")
			if gf_node:
				print("  - GodFlyMesh: visible=", gf_node.visible, " pos=", gf_node.global_position, " mesh=", gf_node.mesh != null)

		# Wait for render to finish on GPU before grabbing texture
		RenderingServer.frame_post_draw.connect(_on_frame_post_draw, CONNECT_ONE_SHOT)

	func _on_frame_post_draw() -> void:
		var vp = get_viewport()
		var tex = vp.get_texture()
		if tex:
			var img = tex.get_image()
			if img and not img.is_empty():
				var out_path = "renders/screenshots/diagnostic_screen.png"
				DirAccess.make_dir_recursive_absolute("renders/screenshots")
				img.save_png(out_path)
				print("[RenderCapture] Saved screenshot to ", out_path, " (", img.get_width(), "x", img.get_height(), ")")
				
				var center_color = img.get_pixel(img.get_width() / 2, img.get_height() / 2)
				var corner_color = img.get_pixel(10, 10)
				var bottom_color = img.get_pixel(img.get_width() / 2, img.get_height() - 50)
				print("  - Center pixel color: ", center_color)
				print("  - Corner pixel color: ", corner_color)
				print("  - Bottom pixel color: ", bottom_color)
			else:
				print("[RenderCapture] Image is empty or null!")
		else:
			print("[RenderCapture] Viewport texture is null!")

func _init() -> void:
	print("[RenderCapture] Starting diagnostic rendering test...")
	var scene_res = load("res://scenes/main.tscn")
	if not scene_res:
		push_error("Cannot load res://scenes/main.tscn")
		quit(1)
		return
	var main_inst = scene_res.instantiate()
	root.add_child(main_inst)
	current_scene = main_inst

	var watcher = FrameWatcher.new()
	root.add_child(watcher)
