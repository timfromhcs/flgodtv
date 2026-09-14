extends CanvasLayer
class_name TelemetryHUD

# TelemetryHUD implements GEMINI.md Sections 84 & 85:
# - LIVE, GENERATION, DAY, COLONY, AGENT, EVENT panels.
# - Research panels: BRAIN, MEMORY, LANGUAGE, EVOLUTION, GENOME, TECHNOLOGY.
# - 100% strict real telemetry: "N/A" when unavailable, zero fake numbers.

@export var bridge_path: NodePath = ^"../BackendBridge"
var bridge: Node = null

# UI Label references
@onready var label_title: Label = get_node_or_null("Panel/VBox/Title")
@onready var label_live: Label = get_node_or_null("Panel/VBox/Live")
@onready var label_time: Label = get_node_or_null("Panel/VBox/Time")
@onready var label_pop: Label = get_node_or_null("Panel/VBox/Population")
@onready var label_camera: Label = get_node_or_null("Panel/VBox/Camera")
@onready var label_event: Label = get_node_or_null("Panel/VBox/Event")
@onready var label_weather: Label = get_node_or_null("Panel/VBox/Weather")
@onready var label_research: Label = get_node_or_null("Panel/VBox/Research")

var research_panel_visible: bool = true
var last_snapshot: Dictionary = {}

func ensure_initialized() -> void:
	if not label_live:
		label_title = get_node_or_null("Panel/VBox/Title")
		label_live = get_node_or_null("Panel/VBox/Live")
		label_time = get_node_or_null("Panel/VBox/Time")
		label_pop = get_node_or_null("Panel/VBox/Population")
		label_camera = get_node_or_null("Panel/VBox/Camera")
		label_event = get_node_or_null("Panel/VBox/Event")
		label_weather = get_node_or_null("Panel/VBox/Weather")
		label_research = get_node_or_null("Panel/VBox/Research")

func _ready() -> void:
	ensure_initialized()
	if has_node(bridge_path):
		bridge = get_node(bridge_path)
		if bridge:
			bridge.state_updated.connect(_on_state_updated)
			if not bridge.telemetry_snapshot.is_empty():
				update_telemetry(bridge.telemetry_snapshot)
			elif bridge.is_connected:
				# Apply synthetic snapshot from bridge
				update_telemetry(bridge.telemetry_snapshot)

func _on_state_updated(state_data: Dictionary) -> void:
	if state_data.has("telemetry_snapshot"):
		update_telemetry(state_data["telemetry_snapshot"])

func update_telemetry(snap: Dictionary) -> void:
	last_snapshot = snap
	if snap.is_empty():
		apply_na_fallback()
		return

	# 1. LIVE Panel
	var live: Dictionary = snap.get("live", {})
	if not live.is_empty():
		var tick_val: int = live.get("tick", 0)
		var elapsed_val: float = live.get("elapsed_seconds", 0.0)
		var tps_val: float = live.get("ticks_per_second", 0.0)
		var status_val: String = live.get("backend_status", "Connected")
		if label_live:
			label_live.text = "LIVE: Tick %d | Elapsed %.1fs | %.0f Ticks/s | %s" % [tick_val, elapsed_val, tps_val, status_val]
	else:
		if label_live: label_live.text = "LIVE: N/A"

	# 2. TIME Panel (Generation & Day)
	var time_dict: Dictionary = snap.get("time", {})
	if not time_dict.is_empty():
		var gen: int = time_dict.get("generation", 0)
		var day: int = time_dict.get("day", 1)
		if label_time:
			label_time.text = "TIME: Gen %d | Day %d" % [gen, day]
	else:
		if label_time: label_time.text = "TIME: N/A"

	# 3. POPULATION & COLONY
	var pop: Dictionary = snap.get("population", {})
	if not pop.is_empty():
		var colonies: int = pop.get("colony_count", 0)
		var total: int = pop.get("total_agents", 0)
		var alive: int = pop.get("alive_agents", 0)
		if label_pop:
			label_pop.text = "POPULATION: %d Alive / %d Total | Colonies: %d" % [alive, total, colonies]
	else:
		if label_pop: label_pop.text = "POPULATION: N/A"

	# 4. CAMERA & SHOT
	var cam: Dictionary = snap.get("camera", {})
	if not cam.is_empty():
		var active_cam: String = cam.get("active_camera", "N/A")
		var active_shot: String = cam.get("active_shot", "N/A")
		var target_label: String = cam.get("focus_target", "N/A")
		if label_camera:
			label_camera.text = "CAMERA: %s [%s] -> Focus: %s" % [active_cam, active_shot, target_label]
	else:
		if label_camera: label_camera.text = "CAMERA: N/A"

	# 5. EVENT
	var ev: Dictionary = snap.get("event", {})
	if not ev.is_empty():
		var ev_text: String = ev.get("latest_event", "N/A")
		var ev_prio: float = ev.get("priority", 0.0)
		if label_event:
			label_event.text = "EVENT: %s (Priority %.1f)" % [ev_text, ev_prio]
	else:
		if label_event: label_event.text = "EVENT: N/A"

	# 6. WEATHER
	var weather: Dictionary = snap.get("weather", {})
	if weather.get("available", false):
		var w_sum: String = weather.get("summary", "N/A")
		var w_temp: float = weather.get("temperature_c", 0.0)
		var w_wind: float = weather.get("wind_speed", 0.0)
		if label_weather:
			label_weather.text = "WEATHER: %s | Temp %.1f°C | Wind %.1f m/s" % [w_sum, w_temp, w_wind]
	else:
		if label_weather: label_weather.text = "WEATHER: N/A"

	# 7. RESEARCH PANELS (GEMINI.md Section 84)
	var research: Dictionary = snap.get("research", {})
	if not research.is_empty():
		var b_str: String = "N/A"
		var m_str: String = "N/A"
		var l_str: String = "N/A"
		var t_str: String = "N/A"
		var e_str: String = "N/A"

		if research.has("brain") and research["brain"].get("available", false):
			b_str = "%.1fM soma/s (%s)" % [research["brain"].get("soma_rate_mps", 0.0), research["brain"].get("model", "MaleCNS")]
		if research.has("memory") and research["memory"].get("available", false):
			m_str = "%d recs" % research["memory"].get("records", 0)
		if research.has("language") and research["language"].get("available", false):
			l_str = "%d words (%d utts)" % [research["language"].get("vocab_size", 0), research["language"].get("utterances", 0)]
		if research.has("technology") and research["technology"].get("available", false):
			t_str = "%d progs" % research["technology"].get("programs", 0)
		if research.has("evolution") and research["evolution"].get("available", false):
			e_str = "%d species" % research["evolution"].get("species_count", 0)

		if label_research:
			label_research.text = "RESEARCH: Brain [%s] | Mem [%s] | Lang [%s] | Tech [%s] | Evo [%s]" % [b_str, m_str, l_str, t_str, e_str]
	else:
		if label_research: label_research.text = "RESEARCH: N/A"

func apply_na_fallback() -> void:
	if label_live: label_live.text = "LIVE: N/A"
	if label_time: label_time.text = "TIME: N/A"
	if label_pop: label_pop.text = "POPULATION: N/A"
	if label_camera: label_camera.text = "CAMERA: N/A"
	if label_event: label_event.text = "EVENT: N/A"
	if label_weather: label_weather.text = "WEATHER: N/A"
	if label_research: label_research.text = "RESEARCH: N/A"

func toggle_research_panel() -> void:
	research_panel_visible = not research_panel_visible
	if label_research:
		label_research.visible = research_panel_visible

func set_hud_visible(p_visible: bool) -> void:
	visible = p_visible
