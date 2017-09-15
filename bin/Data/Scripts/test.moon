assert = require 'luassert'
stub = require 'luassert.stub'
bdd = require 'Test.bdd'
describe = require 'Test.bdd.describe'

snapshot = nil

print("v2")

describe(
	'stubbing userdata'
		
)\beforeEach( () ->
	snapshot = assert\snapshot()
	
)\afterEach( () ->
	snapshot\revert()
	
)\it('successfully stubs userdata', () ->
	
	s = stub(getmetatable(input), "SetMouseVisible")
	
	input\SetMouseVisible(true)

	assert.is_not_nil(getmetatable(input.SetMouseVisible))
	assert.stub(s).was.called_with(input, true)
		
)\it('removes the stub when done', () ->
	
	assert.is_nil(getmetatable(input.SetMouseVisible))
)


scene = nil

describe(
	'construct a surface'
)\beforeEach( () ->
	scene = Scene()

	scene\CreateComponent("Octree")
	
	cameraNode = scene\CreateChild("Camera")	
	cameraNode.position = Vector3(0.0, 10.0, -50.0)
	camera = cameraNode\CreateComponent("Camera")
	viewport = Viewport\new(scene_, cameraNode\GetComponent("Camera"))
	renderer\SetViewport(0, viewport) 
	
	lightNode = scene\CreateChild("DirectionalLight")
	lightNode.direction = Vector3(0.6, -1.0, 0.8)
	light = lightNode\CreateComponent("Light")
	light.lightType = LIGHT_DIRECTIONAL
		
)\afterEach( () ->
	scene = nil
	 
)\it('can be constructed', () ->
	node = scene\CreateChild("Surface")
	surface = UrFelt.UrSurface.new(Vector3(16, 16, 16), Vector3(8, 8, 8), node)
	
	assert.are.equal(type(surface), "userdata", "surface should be userdata")

)\it('can spawn a seed surface', () ->
	node = scene\CreateChild("Surface")
	surface = UrFelt.UrSurface.new(Vector3(16, 16, 16), Vector3(8, 8, 8), node)
	
	surface\seed(Vector3(0,0,0))	
)

success = bdd.runTests()
os.exit(success and 0 or 1)





