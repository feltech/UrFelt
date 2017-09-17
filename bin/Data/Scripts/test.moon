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
	'surface'
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
	@surface = UrFelt.UrSurface.new(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	
	lassert.are.equal(type(@surface), "userdata", "surface should be userdata")

)\it('can spawn a seed', () =>
	node = @scene\CreateChild("Surface")
	@surface = UrFelt.UrSurface.new(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	
	@surface\seed(IntVector3(0,0,0))	
	
)\it('can be rendered', () =>
	node = @scene\CreateChild("Surface")	
	@surface = UrFelt.UrSurface.new(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	@surface\seed(IntVector3(0,0,0))	
	
	@surface\invalidate()
	@surface\polygonise()
	@surface\flush()
	
)\it('can be updated', () =>
	node = @scene\CreateChild("Surface")	
	@surface = UrFelt.UrSurface.new(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
	@surface\seed(IntVector3(0,0,0))	
	@surface\invalidate()

	@surface\update (pos, grid)->
		return -0.5
	
	coroutine.yield()
	
	@surface\polygonise()
	@surface\flush()
	
-- )\it('can raise the surface', () =>
-- 	node = @scene\CreateChild("Surface")	
-- 	@surface = UrFelt.UrSurface.new(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
-- 	@surface\seed(IntVector3(0,0,0))	
-- 	@surface\update (pos, grid)->
-- 		return -1.0	
-- 	@surface\invalidate()
-- 	@surface\polygonise()
-- 	@surface\flush()
-- 	
-- 	ray = Ray(Vector3(0, 0, -10), Vector3(0, 0, 1))
-- 		
-- 	@surface\raise(4, ray)
-- 	coroutine.yield()
)


success = run\runTests()

	
export HandleUpdate = (eventType, eventData)->
	success = run\resumeTests()
-- 	if success ~= nil
-- 		print("Tests completed with success=" .. tostring(success)) 
-- 		os.exit(sucesss and 0 or 1)
	lassert.are.equal("userdata", type(final_scene))
	lassert.are.equal("userdata", type(final_surface))


SubscribeToEvent("Update", "HandleUpdate")

-- os.exit(success and 0 or 1)







