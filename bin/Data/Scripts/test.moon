lassert = require 'luassert'
stub = require 'luassert.stub'
Runner = require 'feltest'
Runner.DEBUG = false
Runner.TIMEOUT = 120
run = Runner()


input\SetMouseVisible(true)
input\SetMouseGrabbed(true)

export camera_node
export final_scene
export final_surface
export MOVE_SPEED
export MOUSE_SENSITIVITY
export yaw
export pitch
export ui_fps_txt
await_finish = nil		


snapshot = nil

run\describe(
	'stubbing userdata'

)\beforeEach( () ->
	snapshot = lassert\snapshot()

)\afterEach( () ->
	snapshot\revert()

)\it('successfully stubs userdata', () ->

	s = stub(getmetatable(input), "SetMouseVisible")

	input\SetMouseVisible(true)

	lassert.is_not_nil(getmetatable(input.SetMouseVisible))
	lassert.stub(s).was.called_with(input, true)

)\it('removes the stub when done', () ->

	lassert.is_nil(getmetatable(input.SetMouseVisible))
)


run\describe(
	'surface synchronous'
)\beforeEach( () =>
	bootstrap_scene(self)

)\afterEach( () =>
	final_scene = @scene
	final_surface = @surface

)\it('can be constructed', () =>
	node = @scene\CreateChild("Surface")
	@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)

	lassert.are.equal(type(@surface), "userdata", "surface should be userdata")

)\it('can spawn a seed', () =>
	node = @scene\CreateChild("Surface")
	@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)

	@surface\seed(IntVector3(0,0,0))

)\it('can be rendered', () =>
	node = @scene\CreateChild("Surface")
	@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	@surface\seed(IntVector3(0,0,0))

	@surface\invalidate()
	@surface\polygonise()
	@surface\flush()

)\it('can be updated', () =>
	node = @scene\CreateChild("Surface")
	@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	@surface\seed(IntVector3(0,0,0))
	@surface\invalidate()

	@surface\update (pos, grid)->
		return -0.5
	@surface\polygonise()
	@surface\flush()
)


run\describe 'asynchronous operations', ()=>		

	@beforeEach () =>
		bootstrap_scene(self)

		node = @scene\CreateChild("Surface")
		@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))
		
	@afterEach () =>
		final_scene = @scene
		final_surface = @surface
		@scene = nil
		@surface = nil

	@it 'block on await', () =>
		finished = false
		expanded = false

		-- No callback.
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))

		-- With a callback.
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1, () ->
			expanded = true
		))

		-- Finish with a polygonise and flush in main thread.
		@surface\enqueue (UrFelt.Op.Polygonise(()->
			@surface\flush()
			finished = true
		))
		lassert.is_false(expanded)
		lassert.is_false(finished)

		coroutine.yield()

		lassert.is_false(finished)
		lassert.is_false(expanded)

		@surface\await()

		lassert.is_true(finished)
		lassert.is_true(expanded)


	@it 'dont block on poll', () =>
		finished = false

		-- With a callback.
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-5, () ->
			@surface\enqueue (UrFelt.Op.Polygonise(()->
				@surface\flush()
				finished = true
			))
		))

		lassert.is_false(finished)

		@surface\poll()
		coroutine.yield()

		lassert.is_false(finished)

		count = 0

		while not finished
			count = count + 1
			@surface\poll()
			coroutine.yield()

		print("Iterations " .. tostring(count))



			
run\describe 'global expansion', ()=>		

	@beforeEach () =>
		bootstrap_scene(self)

		node = @scene\CreateChild("Surface")
		
		@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))
		
		@finished = false
		
		@await_finish = await_finish 
		
	@afterEach () =>
		final_scene = @scene
		final_surface = @surface
		@scene = nil
		@surface = nil
			
	@it 'can fill a sphere', () =>

		@surface\enqueue UrFelt.Op.ExpandToSphere Vector3(-1,-1,-1), 5.0, ()->
			@surface\enqueue UrFelt.Op.Polygonise ()->
				@surface\flush()
				@finished = true

		@await_finish()

	@it 'can fill a box', () =>
		finished = false

		box_start = Vector3(-10,-5,-10)
		box_end = Vector3(10,5,10)

		callback = () ->
			@surface\enqueue UrFelt.Op.Polygonise ()->
				@surface\flush()
				@finished = true

		op = UrFelt.Op.ExpandToBox(box_start, box_end, callback)

		@surface\enqueue(op)
		@await_finish()

	@it 'can move a box to fill a different box', () =>
		finished = false

		box_start = Vector3(3,3,3)
		box_end = Vector3(10,10,10)

		@surface\enqueue UrFelt.Op.ExpandToBox Vector3(3,3,3), Vector3(11,11,11), ()->
			@surface\enqueue UrFelt.Op.ExpandToBox Vector3(-10,-10,-10), Vector3(0,3,0), ()->
				@surface\enqueue UrFelt.Op.Polygonise ()->
					@surface\flush()
					@finished = true

		@await_finish()

	@it 'can move a box to fill a sphere', () =>
		finished = false

		box_start = Vector3(3,3,3)
		box_end = Vector3(10,10,10)

		@surface\enqueue UrFelt.Op.ExpandToBox Vector3(3,3,3), Vector3(11,11,11), ()->
			@surface\enqueue UrFelt.Op.ExpandToSphere Vector3(-5,-5,-5), 7, ()->
				@surface\enqueue UrFelt.Op.Polygonise ()->
					@surface\flush()
					@finished = true

		@await_finish()

	
run\describe 'ray', ()=>

	@beforeEach ()=>
		bootstrap_scene(self)

		node = @scene\CreateChild("Surface")
		@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))
		
	@it 'can cast to surface', ()=>
		ray = @camera\GetScreenRay(0.5, 0.5)

		pos_hit = @surface\ray(ray)

		lassert.is_equal(pos_hit, Vector3(0,0,0))


run\describe 'local expansion', ()=>		

	@beforeEach () =>
		bootstrap_scene(self)

		node = @scene\CreateChild("Surface")		
		@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))
		
		@finished = false
		
		@await_finish = await_finish 
		
	@afterEach () =>
		final_scene = @scene
		final_surface = @surface
		@scene = nil
		@surface = nil
	
	@it "can expand by a constant", ()=>
			
		@surface\enqueue UrFelt.Op.ExpandByConstant -1, ()->
			print("Initial expansion done")
			@surface\enqueue UrFelt.Op.Local.ExpandByConstant(
				Vector3(-1, -1, 1), Vector3(1, 1, 1), -1, ()->
					print("FINISHED")
					@finished = true
			)
			
		@await_finish()
		
		pos_hit = @surface\ray(@camera\GetScreenRay(0.5, 0.5))
		
		lassert.is_equal(pos_hit, Vector3(0,0,-1))
		
		
run\describe 'ExpandToImage', ()=>

	@beforeEach () =>
		bootstrap_scene(self)
		@node = @scene\CreateChild("Surface")

	@afterEach () =>
		final_scene = @scene
		final_surface = @surface
		@scene = nil
		@surface = nil

	@it 'expands to fit until cancelled', ()=>
		print("Loading image")
		finished = false
		start_time = now()

		@surface = UrFelt.UrSurface(IntVector3(200, 200, 200), IntVector3(20, 20, 20), @node)
		@surface\seed(IntVector3(50,50,50))
		@surface\seed(IntVector3(-50,-50,-50))
		@surface\seed(IntVector3(50,50,-50))
		
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))
		
		op = UrFelt.Op.ExpandToImage "brain.hdr", 0.58, 0.2, 0.2, ()->
			finished = true

		@surface\enqueue(op)

		last = now()
		is_rendering = false
		count = 0
		while not finished
			count = count + 1
			if now() - last > 100
				if not is_rendering
					is_rendering = true
					@surface\enqueue UrFelt.Op.Polygonise ()->
						last = now()
						@surface\flush_graphics()
						is_rendering = false
				@surface\poll()

			if now() - start_time > 1000
				op\stop()

			coroutine.yield()
			
		print("Segmentation took " .. tostring(now() - start_time) .. " ms")


export bootstrap_scene = ()=>
	@scene = Scene()
	@scene\CreateComponent("Octree")

	camera_node = @scene\CreateChild("Camera")
	camera_node.position = Vector3(0.0, 0.0, -50.0)
	@camera = camera_node\CreateComponent("Camera")
	viewport = Viewport\new(@scene, camera_node\GetComponent("Camera"))
	renderer\SetViewport(0, viewport)

	light_node = @scene\CreateChild("DirectionalLight")
	light_node.direction = Vector3(0.6, -1.0, 0.8)
	light = light_node\CreateComponent("Light")
	light.lightType = LIGHT_DIRECTIONAL

	point_light = camera_node\CreateComponent("Light")
	point_light.lightType = LIGHT_POINT
	point_light.color = Color(1.0, 1.0, 1.0)
	point_light.specularIntensity = 0.001
	point_light.range = 50	
	
	
await_finish = ()=>
	last = now()
	count = 0
	is_rendering = true
	while not @finished
		count = count + 1
		if now() - last > 100
			if not is_rendering
				is_rendering = true
				@surface\enqueue UrFelt.Op.Polygonise ()->
					last = now()
					@surface\flush()
					is_rendering = false
				
			@surface\poll()
			
		coroutine.yield()
		
	has_final_render = false
	@surface\enqueue UrFelt.Op.Polygonise ()->
		@surface\flush()
		has_final_render = true		

	while not has_final_render
		@surface\poll()
		coroutine.yield()
				
	print("Iterations " .. tostring(count))
	
	
export HandleUpdate = (eventType, eventData)->

	success = run\resumeTests()
	if success ~= nil
		print("Tests completed with " .. if success then "success" else "failure")
		os.exit(sucesss and 0 or 1)

	timeStep = eventData["TimeStep"]\GetFloat()

	ui_fps_txt\SetText("FPS: " .. tostring(1/timeStep))

	-- Use this frame's mouse motion to adjust camera node yaw and pitch.
	if input\IsMouseGrabbed()
		mouseMove = input.mouseMove
		yaw = yaw + MOUSE_SENSITIVITY * mouseMove.x
		pitch = pitch - MOUSE_SENSITIVITY * mouseMove.y
-- 		pitch = Clamp(pitch, -90.0, 90.0)
		camera_node.rotation = Quaternion(pitch, yaw, 0.0)

	-- Read WASD keys and move the camera scene node to the corresponding direction if they are
	-- pressed
	if input\GetKeyDown(KEY_W)
		camera_node\Translate(Vector3(0.0, 1.0, 0.0) * MOVE_SPEED * timeStep)

	if input\GetKeyDown(KEY_S)
		camera_node\Translate(Vector3(0.0, -1.0, 0.0) * MOVE_SPEED * timeStep)

	if input\GetKeyDown(KEY_A)
		camera_node\Translate(Vector3(-1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)

	if input\GetKeyDown(KEY_D)
		camera_node\Translate(Vector3(1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)

	if input\GetKeyDown(KEY_X)
		camera_node\Translate(Vector3(0.0, 0.0, 1.0) * MOVE_SPEED * timeStep)

	if input\GetKeyDown(KEY_Z)
		camera_node\Translate(Vector3(0.0, 0.0, -1.0) * MOVE_SPEED * timeStep)


export HandleKeyDown = (eventType, eventData)->
	key = eventData["Key"]\GetInt()
	-- Close console (if open) or exit when ESC is pressed
	if key == KEY_ESC
		engine\Exit()

	if input\GetKeyDown(KEY_SPACE)
		input\SetMouseVisible(not input\IsMouseVisible())
		input\SetMouseGrabbed(not input\IsMouseVisible())



-- Movement speed as world units per second
MOVE_SPEED = 30.0
-- Mouse sensitivity as degrees per pixel
MOUSE_SENSITIVITY = 0.1
yaw = 0
pitch = 0
input\SetMouseVisible(true)
input\SetMouseGrabbed(false)

ui_fps_txt = ui.root\CreateChild("Text")
ui_fps_txt\SetFont(cache\GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
ui_fps_txt.textAlignment = HA_CENTER
ui_fps_txt.horizontalAlignment = HA_LEFT
ui_fps_txt.verticalAlignment = VA_TOP

SubscribeToEvent("Update", "HandleUpdate")
SubscribeToEvent("KeyDown", "HandleKeyDown")


run\runTests()







