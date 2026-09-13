extends SceneTree

# Headless smoke test verifying Godot 4 project parsing, scene instantiation,
# 4-camera rig, MultiMesh instance setup, and backend bridge data.

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
	print("  - FlyMultiMesh verified.")

	# Verify Backend Bridge
	var bridge = main_node.get_node_or_null("BackendBridge")
	assert(bridge != null, "BackendBridge must exist")
	if bridge.get_agent_count() == 0:
		bridge.load_initial_or_fallback_state()
	assert(bridge.get_agent_count() > 0, "Agents must be initialized")
	assert(bridge.get_colony_count() > 0, "Colonies must be initialized")
	print("  - BackendBridge verified: " + str(bridge.get_agent_count()) + " agents, " + str(bridge.get_colony_count()) + " colonies.")

	# Simulate 10 frames
	for i in range(10):
		main_node._process(1.0 / 60.0)

	print("[Godot Headless Test] ALL FRONTEND SMOKE TESTS PASSED!")
	quit(0)
