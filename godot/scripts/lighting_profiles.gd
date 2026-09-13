extends Node
class_name LightingProfiles

# Lighting quality profiles per GEMINI.md Section 76:
# LOW / MEDIUM / HIGH / CINEMATIC. Every expensive visual feature is optional;
# simulation never depends on the active profile.

enum Profile { LOW, MEDIUM, HIGH, CINEMATIC }

var active_profile: int = Profile.MEDIUM

func apply_profile(p: int) -> void:
	active_profile = clampi(p, Profile.LOW, Profile.CINEMATIC)
	var env := find_env()
	var sun := find_sun()
	match active_profile:
		Profile.LOW:
			if env and env.environment:
				env.environment.glow_enabled = false
				env.environment.tonemap_mode = Environment.TONE_MAPPER_LINEAR
				env.environment.ssao_enabled = false
				env.environment.sdfgi_enabled = false
			if sun:
				sun.shadow_enabled = false
		Profile.MEDIUM:
			if env and env.environment:
				env.environment.glow_enabled = false
				env.environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
			if sun:
				sun.shadow_enabled = true
		Profile.HIGH:
			if env and env.environment:
				env.environment.glow_enabled = true
				env.environment.tonemap_mode = Environment.TONE_MAPPER_ACES
			if sun:
				sun.shadow_enabled = true
		Profile.CINEMATIC:
			if env and env.environment:
				env.environment.glow_enabled = true
				env.environment.tonemap_mode = Environment.TONE_MAPPER_ACES
				env.environment.ssao_enabled = true
			if sun:
				sun.shadow_enabled = true
	print("[LightingProfiles] Active profile: %s." % profile_name())

func profile_name() -> String:
	match active_profile:
		Profile.LOW:
			return "LOW"
		Profile.MEDIUM:
			return "MEDIUM"
		Profile.HIGH:
			return "HIGH"
		_:
			return "CINEMATIC"

func find_env() -> WorldEnvironment:
	var root := get_tree().current_scene if is_inside_tree() and get_tree() else get_parent()
	if root and root.has_node("WorldEnvironment"):
		return root.get_node("WorldEnvironment") as WorldEnvironment
	if get_parent() and get_parent().has_node("WorldEnvironment"):
		return get_parent().get_node("WorldEnvironment") as WorldEnvironment
	return null

func find_sun() -> DirectionalLight3D:
	var root := get_tree().current_scene if is_inside_tree() and get_tree() else get_parent()
	if root and root.has_node("DirectionalLight3D"):
		return root.get_node("DirectionalLight3D") as DirectionalLight3D
	return null
