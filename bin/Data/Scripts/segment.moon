DebugScene = require 'debug_scene'

surface = nil
debug_scene = DebugScene()
debug_scene\recreate()

node = debug_scene.scene\CreateChild("Surface")
surface = UrFelt.UrSurface(
	IntVector3(200, 200, 200), IntVector3(10, 10, 10), node
)
surface\seed(IntVector3(40, 0, 30))
surface\seed(IntVector3(-40, 0, 30))
surface\seed(IntVector3(40, 0, -30))
surface\seed(IntVector3(-40, 0, -30))

surface\expand_by_constant(-1)


last_render = now()
last_poll = last_render
is_rendering = false
saved = false
started = false

ui_txt = ui.root\CreateChild("Text")
ui_txt.textAlignment = HA_CENTER
ui_txt.horizontalAlignment = HA_CENTER
ui_txt.verticalAlignment = VA_CENTER
ui_txt\SetFont(cache\GetResource("Font", "Fonts/Anonymous Pro.ttf"), 20)
ui_txt\SetText("Prese ENTER to start")


on_update = (eventType, eventData)->
	if surface ~= nil
		if now() - last_poll > 1000/20
			last_poll = now()
			surface\poll()

		if now() - last_render > 1000/10
			last_render = now()
			if not is_rendering
				is_rendering = true
				surface\polygonise ->
					surface\flush_graphics()
					is_rendering = false

	if saved
		engine\Exit()


on_key = (eventType, eventData)->
	key = eventData["Key"]\GetInt()
	if key == KEY_RETURN
		if not started
			filepath = GetFileSystem().GetProgramDir() .. "/Data/brain.hdr"
			surface\transform_to_image(filepath, 0.58, 0.2, 0.2)
			started = true
			ui_txt\SetText("")
			ui_txt = nil
		else
			print("Saving ...")
			surface\save "Data/brain.bin.gz", ->
				print("... saved")


debug_scene\subscribe_to_update(on_update)
debug_scene\subscribe_to_key_down(on_key)

