DebugScene = require 'debug_scene'

class CollisionTest extends DebugScene
    new: =>
        super()
		@_last_poly = now()
		@_last_poll = now()
        @recreate()

		@_camera_node.position = Vector3(0, 20, -20)
		@_camera_node.rotation = Quaternion(0.9, 0.37, 0, 0)
        @_camera_node.rotation\Normalize()

		floor_node = @scene\CreateChild("Floor")
		floor_node.position = Vector3(0.0, -200, 0.0)
		floor_node.scale = Vector3(1000.0, 1.0, 1000.0)
		floor_object = floor_node\CreateComponent("StaticModel")
		floor_object.model = cache\GetResource("Model", "Models/Box.mdl")
		floor_object.material = cache\GetResource("Material", "Materials/StoneTiled.xml")
		body = floor_node\CreateComponent("RigidBody")
		shape = floor_node\CreateComponent("CollisionShape")
		shape\SetBox(Vector3(1.0, 1.0, 1.0))

        @log("Creating surface")
        node = @scene\CreateChild("Surface")
        @surface = UrFelt.UrSurface(
            IntVector3(200, 200, 200), IntVector3(10, 10, 10), node
        )
        @log("Creating box")
        @surface\seed(IntVector3(0, 0, 0))
        @surface\expand_by_constant(-1)
        @surface\transform_to_box Vector3(-5, -5, -5), Vector3(5, 5, 5), () ->
			box_node = @scene\CreateChild("SmallBox")
			box_node.position = Vector3(3, 8, -3)
			box_node\SetScale(1)
			box_object = box_node\CreateComponent("StaticModel")
			box_object.model = cache\GetResource("Model", "Models/Box.mdl")
			box_object.material = cache\GetResource("Material", "Materials/StoneTiled.xml")
			box_object.castShadows = true
			-- Create physics components, use a smaller mass also
			body = box_node\CreateComponent("RigidBody")
			body.mass = 0.25
			body.friction = 0.75
			body.restitution = 0.1
			shape = box_node\CreateComponent("CollisionShape")
			shape\SetBox(Vector3(1.0, 1.0, 1.0))

	_on_update: (event_type, event_data)=>
		super(event_type, event_data)
		current_time = now()

		-- 30ms between updates of polygonisation
		if current_time - @_last_poly > 30
			@_last_poly = now()
			@surface\polygonise ->
				@surface\flush()

		-- 5ms between polling for Lua callbacks to surface Ops.
		if current_time - @_last_poll > 5
			@surface\poll()

scene = CollisionTest()
