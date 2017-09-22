lassert = require 'luassert'
stub = require 'luassert.stub'
Runner = require 'feltest'
Runner.DEBUG = false
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
	viewport = Viewport\new(@scene, cameraNode\GetComponent("Camera"))
	renderer\SetViewport(0, viewport) 
	
	lightNode = @scene\CreateChild("DirectionalLight")
	lightNode.direction = Vector3(0.6, -1.0, 0.8)
	light = lightNode\CreateComponent("Light")
	light.lightType = LIGHT_DIRECTIONAL
		
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


run\describe 'surface asynchronous', ()=>
	
	@beforeEach () =>
		@scene = Scene()
		@scene\CreateComponent("Octree")
		
		cameraNode = @scene\CreateChild("Camera")	
		cameraNode.position = Vector3(0.0, 0.0, -10.0)
		camera = cameraNode\CreateComponent("Camera")
		viewport = Viewport\new(@scene, cameraNode\GetComponent("Camera"))
		renderer\SetViewport(0, viewport) 
		
		lightNode = @scene\CreateChild("DirectionalLight")
		lightNode.direction = Vector3(0.6, -1.0, 0.8)
		light = lightNode\CreateComponent("Light")
		light.lightType = LIGHT_DIRECTIONAL
		
		node = @scene\CreateChild("Surface")	
		@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
		@surface\seed(IntVector3(0,0,0))	
		
	@afterEach () =>
		final_scene = @scene
		final_surface = @surface
		
	
	@it 'can be updated and awaited', () =>
		flushed = false
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
			flushed = true
		))		
		lassert.is_false(expanded)
		lassert.is_false(flushed)
		
		coroutine.yield()
		
		lassert.is_false(flushed)
		lassert.is_false(expanded)
		
		@surface\await()
		
		lassert.is_true(flushed)
		lassert.is_true(expanded)
		
		
	@it 'can be updated over multiple event loops', () =>
		flushed = false
		
		-- With a callback.
		@surface\enqueue (UrFelt.Op.ExpandByConstant(-5, () ->
			@surface\enqueue (UrFelt.Op.Polygonise(()->
				@surface\flush()
				flushed = true
			))	
		))
	
		lassert.is_false(flushed)
		
		@surface\poll()
		coroutine.yield()
	
		lassert.is_false(flushed)
		
		count = 0
		
		while not flushed
			count = count + 1
			@surface\poll()
			coroutine.yield()
			
		print("Iterations: " .. tostring(count))
		lassert.is_true(flushed)

	@it 'expands to fill a box', () =>
		flushed = false
		
		-- With a callback.
		@surface\enqueue (UrFelt.Op.ExpandToBox(Vector3(-10,-5,-10), Vector3(10,5,10) () ->
			@surface\enqueue (UrFelt.Op.Polygonise(()->
				@surface\flush()
				flushed = true
			))	
		))
	
		while not flushed
			count = count + 1
			@surface\poll()
			coroutine.yield()
			
		print("Iterations: " .. tostring(count))
		lassert.is_true(flushed)


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







