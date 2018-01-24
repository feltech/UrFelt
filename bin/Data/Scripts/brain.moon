DebugScene = require 'debug_scene'
FSM = require 'fsm'


local State, Idle, Load, Generate, Running


class BrainApp extends DebugScene
	new: =>
		@surface = nil
		@_state = Idle()
		super()

		@fsm = FSM.create({
			initial: "idle",
			events: {
				{name: "load", from: "idle", to: "load_from_disk"},
				{name: "loaded", from: "load_from_disk", to: "generate"},
				{name: "generated", from: "generate", to: "running"},
				{name: "regenerate", from: "running", to: "generate"}
			},
			callbacks: {
				on_enter_idle: (fsm, event, fro, to)-> @_transition(Idle)

				on_enter_load_from_disk: (fsm, event, fro, to)-> @_transition(Load)

				on_enter_generate: (fsm, event, fro, to, surface)-> @_transition(Generate)

				on_enter_running: (fsm, event, fro, to)-> @_transition(Running)
			}
		})

		@recreate()

		-- Create a floor object, 1000 x 1000 world units. Adjust position so that the ground is at
		-- zero Y
		floor_node = @scene\CreateChild("Floor")
		floor_node.position = Vector3(0.0, -100, 0.0)
		floor_node.scale = Vector3(1000.0, 1.0, 1000.0)
		floor_object = floor_node\CreateComponent("StaticModel")
		floor_object.model = cache\GetResource("Model", "Models/Box.mdl")
		floor_object.material = cache\GetResource("Material", "Materials/StoneTiled.xml")

		-- Make the floor physical by adding RigidBody and CollisionShape components. The
		-- RigidBody's default parameters make the object static (zero mass). Note that a
		-- CollisionShape by itself will not participate in the physics simulation
		body = floor_node\CreateComponent("RigidBody")
		shape = floor_node\CreateComponent("CollisionShape")
		-- Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene
		-- node scale, so the rendering and physics representation sizes should match (the box
		-- model is also 1 x 1 x 1.)
		shape\SetBox(Vector3(1.0, 1.0, 1.0))

		@fsm.load()

	_transition: (cls)=>
		@_state\tear_down()
		@_state = cls(self)

	_on_update: (event_type, event_data)=>
		super(event_type, event_data)
		@_state\poll()


class State
	new: (app)=>
		print("Entering " .. @@__name)
		@_app = app

	tear_down: =>
		return

	poll: =>
		return


class Idle extends State
	new: (app)=>
		super(app)
	poll: =>
		super()


class LoadingBase extends State
	new: (app)=>
		super(app)
		@_ui_txt = ui.root\CreateChild("Text")
		@_ui_txt.textAlignment = HA_CENTER
		@_ui_txt.horizontalAlignment = HA_CENTER
		@_ui_txt.verticalAlignment = VA_CENTER
		@_ui_txt\SetFont(cache\GetResource("Font", "Fonts/Anonymous Pro.ttf"), 20)

	tear_down: =>
		@_ui_txt\Remove()


class Load extends LoadingBase
	new: (app, brain_scene)=>
		super(app)
		@_ui_txt\SetText("Loading from disk...")

		node = @_app.scene\CreateChild("Surface")
		@_loader = UrFelt.UrSurface.load("brain.bin.gz", node)

	poll: =>
		super()
		if @_loader\ready()
			@_app.surface = @_loader\get()
			@_app.fsm.loaded()


class Generate extends LoadingBase
	new: (app, surface)=>
		super(app)
		@_ui_txt\SetText("Regenerating...")

		@_flushed = false
		@_app.surface\invalidate()
		@_app.surface\polygonise ->
			@_app.surface\flush()
			@_flushed = true

	poll: =>
		super()
		@_app.surface\poll()
		if @_flushed
			@_app.fsm.generated()


class Running extends State
	OBJECT_VELOCITY: 20

	new: (app)=>
		super(app)
		@_last_poly = now()
		@_last_poll = now()
		@_op = nil
		@_op_start_time = nil

	poll: =>
		if input\GetKeyPress(KEY_R)
			@_app.fsm.regenerate()
			return

		if input\IsMouseGrabbed()
			if input\GetMouseButtonPress(MOUSEB_LEFT) then @_throw_box()

		elseif input\IsMouseVisible()
			if @_op_start_time == nil
				if input\GetMouseButtonDown(MOUSEB_LEFT) then @_lower_surface()
				else if input\GetMouseButtonDown(MOUSEB_RIGHT) then @_raise_surface()

		current_time = now()

		-- 30ms between updates of polygonisation
		if current_time - @_last_poly > 30
			@_last_poly = now()
			@_app.surface\polygonise ->
				@_app.surface\flush()

		-- 5ms between polling for Lua callbacks to surface Ops.
		if current_time - @_last_poll > 5
			@_app.surface\poll()

		if @_op_start_time ~= nil
			if current_time - @_op_start_time > 50
				@_op\stop()

	_raise_surface: (direction)=>
		pos_hit = @_raycast()
		if pos_hit == nil
			return

		@_op_start_time = now()
		@_op = @_app.surface\attract_to_sphere pos_hit, 3, ->
			@_op_start_time = nil

	_lower_surface: (direction)=>
		pos_hit = @_raycast()
		if pos_hit == nil
			return

		@_op_start_time = now()
		@_op = @_app.surface\repel_from_sphere pos_hit, 3, ->
			@_op_start_time = nil

	_raycast: ()=>
		mouse_pos = input\GetMousePosition()
		screen_coordX = mouse_pos.x / graphics\GetWidth()
		screen_coordY = mouse_pos.y / graphics\GetHeight()

		if screen_coordX < 0 or screen_coordX > 1 or screen_coordY < 0 or screen_coordY > 1
			print("Outside bounds (" .. screen_coordX .. ", " .. screen_coordY .. ")")
			return

		ray = @_app.camera\GetScreenRay(screen_coordX, screen_coordY)
		pos_hit = @_app.surface\ray(ray)

		if pos_hit == nil
			return

		return pos_hit

	_throw_box: =>
		-- Create a smaller box at camera position
		camera_node =  @_app.camera\GetNode()
		box_node = @_app.scene\CreateChild("SmallBox")
		box_node.position = camera_node.position
		box_node.rotation = camera_node.rotation
		box_node\SetScale(1)
		box_object = box_node\CreateComponent("StaticModel")
		box_object.model = cache\GetResource("Model", "Models/Box.mdl")
		box_object.material = cache\GetResource("Material", "Materials/StoneTiled.xml")
		box_object.castShadows = true

		-- Create physics components, use a smaller mass also
		body = box_node\CreateComponent("RigidBody")
		body.mass = 0.25
		body.friction = 0.75
		body.restitution = 0
		shape = box_node\CreateComponent("CollisionShape")
		shape\SetBox(Vector3(1.0, 1.0, 1.0))

		-- Set initial velocity for the RigidBody based on camera forward vector. Add also a
		-- slight up component to overcome gravity better
		body.linearVelocity = camera_node.rotation * Vector3(0.0, 0.25, 1.0) * @OBJECT_VELOCITY



scene = BrainApp()

