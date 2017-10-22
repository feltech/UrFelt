DebugScene = require 'debug_scene'

surface = nil
debug_scene = DebugScene()
debug_scene\recreate()

node = debug_scene.scene\CreateChild("Surface")
surface = UrFelt.UrSurface.load("brain.bin.gz", node)\get()
surface\invalidate()
surface\polygonise()
surface\await()
surface\flush()

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

debug_scene\subscribe_to_update(on_update)
