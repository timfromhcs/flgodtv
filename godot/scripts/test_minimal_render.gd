extends SceneTree

# Phase 2: Minimal Godot Rendering Smoke Test
# Proves that Godot Forward+ Vulkan can visibly render a deterministic test object
# (BoxMesh with bright red material under directional light) and produce verifiable
# non-grey pixels.

class MinimalWatcher extends Node:
	var frames: int = 15

	func _process(_delta: float) -> void:
		frames -= 1
		if frames <= 0:
			RenderingServer.frame_post_draw.connect(_on_drawn, CONNECT_ONE_SHOT)

	func _on_drawn() -> void:
		var vp = get_viewport()
		var tex = vp.get_texture()
		assert(tex != null, "Viewport texture must not be null")
		var img = tex.get_image()
		assert(img != null and not img.is_empty(), "Viewport image must not be empty")

		var out_dir = ProjectSettings.globalize_path("res://../renders/screenshots")
		DirAccess.make_dir_recursive_absolute(out_dir)
		var out_file = out_dir + "/minimal_render_proof.png"
		img.save_png(out_file)
		print("[MinimalRender] Saved proof screenshot to: ", out_file)

		# Verify center pixel is predominantly RED (the test cube)
		var center_col = img.get_pixel(img.get_width() / 2, img.get_height() / 2)
		print("[MinimalRender] Center pixel: ", center_col)
		# Red channel should be clearly dominant
		assert(center_col.r > 0.4, "Center pixel red channel must be > 0.4")
		assert(center_col.r > center_col.b, "Center pixel red must exceed blue")
		print("[MinimalRender] VERIFICATION SUCCESSFUL: Non-grey deterministic 3D object rendered!")
		get_tree().quit(0)

func _init() -> void:
	print("[MinimalRender] Initializing minimal 3D test scene...")
	var root_3d = Node3D.new()
	root_3d.name = "MinimalScene"
	root.add_child(root_3d)

	# 1. WorldEnvironment
	var env_node = WorldEnvironment.new()
	var env = Environment.new()
	env.background_mode = Environment.BG_SKY
	var sky = Sky.new()
	var sky_mat = ProceduralSkyMaterial.new()
	sky_mat.sky_top_color = Color(0.2, 0.5, 0.9, 1.0)
	sky_mat.sky_horizon_color = Color(0.7, 0.8, 0.9, 1.0)
	sky.sky_material = sky_mat
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env_node.environment = env
	root_3d.add_child(env_node)

	# 2. DirectionalLight3D
	var light = DirectionalLight3D.new()
	light.position = Vector3(5, 10, 5)
	light.look_at_from_position(Vector3(5, 10, 5), Vector3.ZERO, Vector3.UP)
	light.shadow_enabled = true
	root_3d.add_child(light)

	# 3. Camera3D
	var cam = Camera3D.new()
	cam.position = Vector3(0, 1.5, 4.0)
	cam.look_at_from_position(Vector3(0, 1.5, 4.0), Vector3(0, 0.5, 0), Vector3.UP)
	cam.current = true
	cam.fov = 60.0
	root_3d.add_child(cam)

	# 4. MeshInstance3D (Bright Red Cube)
	var mesh_inst = MeshInstance3D.new()
	var box = BoxMesh.new()
	box.size = Vector3(1.2, 1.2, 1.2)
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0.9, 0.1, 0.1, 1.0) # Bright Red
	mat.roughness = 0.3
	box.material = mat
	mesh_inst.mesh = box
	mesh_inst.position = Vector3(0, 0.5, 0)
	root_3d.add_child(mesh_inst)

	# Attach frame watcher
	var watcher = MinimalWatcher.new()
	root_3d.add_child(watcher)
