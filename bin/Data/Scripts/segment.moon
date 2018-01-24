DebugScene = require 'debug_scene'

surface = nil
debug_scene = DebugScene()
debug_scene\recreate()

node = debug_scene.scene\CreateChild("Surface")
surface = UrFelt.UrSurface(
	IntVector3(200, 200, 200), IntVector3(10, 10, 10), node
)
surface\seed(IntVector3(40,0,30))
surface\seed(IntVector3(-50,0 -20))
surface\seed(IntVector3(25, -70,-50))
surface\seed(IntVector3(25, 60,35))

surface\expand_by_constant(-1)

surface\transform_to_image("brain.hdr", 0.58, 0.2, 0.2)


last_render = now()
last_poll = last_render
is_rendering = false
saved = false

on_update = (eventType, eventData)->
	if surface ~= nil
		if now() - last_poll > 100
			last_poll = now()
			surface\poll()

		if now() - last_render > 5000
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
		print("Saving ...")
		surface\save "brain.bin.gz", ->
			print("... saved")


debug_scene\subscribe_to_update(on_update)
debug_scene\subscribe_to_key_down(on_key)

