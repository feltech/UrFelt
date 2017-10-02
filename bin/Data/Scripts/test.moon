lassert = require 'luassert'
stub = require 'luassert.stub'
Runner = require 'feltest'
Runner.DEBUG = false
Runner.TIMEOUT = 120
run = Runner()

snapshot = nil

print("v4")

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

input\SetMouseVisible(true)
input\SetMouseGrabbed(true)

export final_scene
export final_surface

run\describe(
	'surface synchronous'
)\beforeEach( () =>
	@scene = Scene()
	@scene\CreateComponent("Octree")
	
	cameraNode = @scene\CreateChild("Camera")	
	cameraNode.position = Vector3(0.0, 0.0, -10.0)
	camera = cameraNode\CreateComponent("Camera")
	viewport = Viewport(@scene, cameraNode\GetComponent("Camera"))
	renderer\SetViewport(0, viewport) 
	
	lightNode = @scene\CreateChild("DirectionalLight")
	lightNode.direction = Vector3(0.6, -1.0, 0.8)
	light = lightNode\CreateComponent("Light")
	light.lightType = LIGHT_DIRECTIONAL
		
)\afterEach( () =>
	collectgarbage("collect")
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


run\describe 'surface asynchronous', ()=>
	
	@beforeEach () =>
		@scene = Scene()
		@scene\CreateComponent("Octree")
		
		cameraNode = @scene\CreateChild("Camera")	
		cameraNode.position = Vector3(0.0, 0.0, -50.0)
		camera = cameraNode\CreateComponent("Camera")
		viewport = Viewport\new(@scene, cameraNode\GetComponent("Camera"))
		renderer\SetViewport(0, viewport) 
		
		lightNode = @scene\CreateChild("DirectionalLight")
		lightNode.direction = Vector3(0.6, -1.0, 0.8)
		light = lightNode\CreateComponent("Light")
		light.lightType = LIGHT_DIRECTIONAL
		
		node = @scene\CreateChild("Surface")	
		@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))	
		
	@afterEach () =>
		collectgarbage("collect")
		final_scene = @scene
		final_surface = @surface
		@scene = nil
		@surface = nil
		
	
	@it 'can be updated and awaited', () =>
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
		
		
	@it 'can be updated over multiple event loops', () =>
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
			
		print("Iterations: " .. tostring(count))
		lassert.is_true(finished)

	@it 'expands to fill a box', () =>
		finished = false
		
		box_start = Vector3(-10,-5,-10)
		box_end = Vector3(10,5,10)
		
		@surface\invalidate()
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))
		
		callback = () ->
			@surface\invalidate()
			@surface\enqueue UrFelt.Op.Polygonise ()->
				@surface\flush()
				finished = true
		
		op = UrFelt.Op.ExpandToBox(box_start, box_end, callback)
		
		@surface\enqueue(op)
		
		last = os.time()
		count = 0
		while not finished
			count = count + 1
			@surface\poll()
			if os.time() - last > 0.05
				last = os.time()
				@surface\enqueue UrFelt.Op.Polygonise ()->
					@surface\flush()
			coroutine.yield()
			
		print("Iterations: " .. tostring(count))
		lassert.is_true(finished)

	@it 'moves to fill a box', () =>
		finished = false
		
		box_start = Vector3(3,3,3)
		box_end = Vector3(10,10,10)
		
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))
		
		@surface\enqueue UrFelt.Op.ExpandToBox Vector3(3,3,3), Vector3(11,11,11), ()->
			print("Done box 1")
			@surface\enqueue UrFelt.Op.ExpandToBox Vector3(-10,-10,-10), Vector3(0,3,0), ()->
				print("Done box 2")
				@surface\enqueue UrFelt.Op.Polygonise ()->
					@surface\flush()
					finished = true
			
		
		last = os.time()
		count = 0
		while not finished
			count = count + 1
			if os.time() - last > 0.05
				@surface\enqueue UrFelt.Op.Polygonise ()->
					last = os.time()
					@surface\flush()
			@surface\poll()
			coroutine.yield()
			
		print("Iterations: " .. tostring(count))
		lassert.is_true(finished)
		
	@it 'expands to fit an image', ()=>
		print("Loading image")
		finished = false
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-1))
		
		@surface\enqueue UrFelt.Op.ExpandToImage "brain.hdr", 0.3, 0.1, 0.1, ()->
			@surface\enqueue UrFelt.Op.Polygonise ()->
				@surface\flush()
				finished = true
				
		last = os.time()
		count = 0
		while not finished
			count = count + 1
			if os.time() - last > 0.05
				@surface\enqueue UrFelt.Op.Polygonise ()->
					last = os.time()
					@surface\flush()
			@surface\poll()
			coroutine.yield()
		
		
success = run\runTests()

	
export HandleUpdate = (eventType, eventData)->
	
	success = run\resumeTests()
-- 	if success ~= nil
-- 		print("Tests completed with success=" .. tostring(success)) 
-- 		os.exit(sucesss and 0 or 1)
-- 	lassert.are.equal("userdata", type(final_scene))
-- 	lassert.are.equal("userdata", type(final_surface))


SubscribeToEvent("Update", "HandleUpdate")

-- os.exit(success and 0 or 1)







