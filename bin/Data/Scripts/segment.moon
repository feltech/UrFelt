DebugScene = require 'debug_scene'

surface = nil
debug_scene = DebugScene()
debug_scene\recreate()

node = debug_scene.scene\CreateChild("Surface")
surface = UrFelt.UrSurface(
	IntVector3(200, 200, 200), IntVector3(20, 20, 20), node
)
surface\seed(IntVector3(-50,-50,-50))
surface\seed(IntVector3(-50,-50, 50))
surface\seed(IntVector3(-50, 50,-50))
surface\seed(IntVector3(-50, 50, 50))
surface\seed(IntVector3( 50,-50,-50))
surface\seed(IntVector3( 50,-50, 50))
surface\seed(IntVector3( 50, 50,-50))
surface\seed(IntVector3( 50, 50, 50))

surface\expand_by_constant(-1)

surface\transform_to_image("brain.hdr", 0.58, 0.2, 0.2)


last = now()
is_rendering = false
saved = false

on_update = (eventType, eventData)->
	if surface ~= nil
		surface\poll()

		if now() - last > 5000
			last = now()
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
		surface\save "brain2.bin.gz", ->
			print("... saved")


debug_scene\subscribe_to_update(on_update)
debug_scene\subscribe_to_key_down(on_key)

