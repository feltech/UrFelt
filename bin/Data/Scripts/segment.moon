bootstrap_scene = require 'debug_scene'

surface = nil
scene, camera = bootstrap_scene()

run = ->
	node = scene\CreateChild("Surface")
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

export poll = (eventType, eventData)->
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


export handleSaveAndExit = (eventType, eventData)->
	key = eventData["Key"]\GetInt()
	-- Close console (if open) or exit when ESC is pressed
	if key == KEY_ENTER
		surface\save "/tmp/urfelt.brain.bin"


SubscribeToEvent("Update", "poll")
SubscribeToEvent("KeyDown", "handleSaveAndExit")



-- run()
