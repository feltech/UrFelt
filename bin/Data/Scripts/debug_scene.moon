class DebugScene
	-- Movement speed as world units per second
	MOVE_SPEED: 30.0
	-- Mouse sensitivity as degrees per pixel
	MOUSE_SENSITIVITY: 0.1

	new: =>
		@_ui_fps_txt = ui.root\CreateChild("Text")
		@_ui_fps_txt\SetFont(cache\GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
		@_ui_fps_txt.textAlignment = HA_CENTER
		@_ui_fps_txt.horizontalAlignment = HA_LEFT
		@_ui_fps_txt.verticalAlignment = VA_TOP

		@_yaw = 0
		@_pitch = 0

		@_update_cbs = {}
		@_key_down_cbs = {}

		input\SetMouseVisible(true)
		input\SetMouseGrabbed(false)

		SubscribeToEvent "Update", (event_type, event_data) ->
			@_on_update(event_type, event_data)
		SubscribeToEvent "KeyDown", (event_type, event_data) ->
			@_on_key_down(event_type, event_data)

	recreate: =>
		@scene = Scene()
		@scene\CreateComponent("Octree")

		@_camera_node = @scene\CreateChild("Camera")
		@_camera_node.position = Vector3(0.0, 0.0, -50.0)
		@camera = @_camera_node\CreateComponent("Camera")
		viewport = Viewport\new(@scene, @_camera_node\GetComponent("Camera"))
		renderer\SetViewport(0, viewport)

		light_node = @scene\CreateChild("DirectionalLight")
		light_node.direction = Vector3(0.6, -1.0, 0.8)
		light = light_node\CreateComponent("Light")
		light.lightType = LIGHT_DIRECTIONAL

		point_light = @_camera_node\CreateComponent("Light")
		point_light.lightType = LIGHT_POINT
		point_light.color = Color(1.0, 1.0, 1.0)
		point_light.specularIntensity = 0.001
		point_light.range = 50

	subscribe_to_update: (fn) => table.insert(@_update_cbs, fn)

	subscribe_to_key_down: (fn) => table.insert(@_key_down_cbs, fn)

	_on_update: (event_type, event_data)=>
		time_step = event_data["TimeStep"]\GetFloat()
		@_update_FPS_display(time_step)
		@_update_camera_movement(time_step)
		for fn in *@_update_cbs
			fn(event_type, event_data)

	_update_FPS_display: (time_step)=>
		@_ui_fps_txt\SetText("FPS: " .. tostring(1/time_step))

	_update_camera_movement: (time_step)=>
		-- Use this frame's mouse motion to adjust camera node _yaw and _pitch.
		if input\IsMouseGrabbed()
			mouseMove = input.mouseMove
			yaw = @MOUSE_SENSITIVITY * mouseMove.x
			pitch = -@MOUSE_SENSITIVITY * mouseMove.y

			@_camera_node.rotation = @_camera_node.rotation*Quaternion(pitch, yaw, 0)
			@_camera_node.rotation\Normalize()


		-- Read WASD keys and move the camera scene node to the corresponding direction if they are
		-- pressed
		if input\GetKeyDown(KEY_W)
			@_camera_node\Translate(Vector3(0.0, 1.0, 0.0) * @MOVE_SPEED * time_step)

		if input\GetKeyDown(KEY_S)
			@_camera_node\Translate(Vector3(0.0, -1.0, 0.0) * @MOVE_SPEED * time_step)

		if input\GetKeyDown(KEY_A)
			@_camera_node\Translate(Vector3(-1.0, 0.0, 0.0) * @MOVE_SPEED * time_step)

		if input\GetKeyDown(KEY_D)
			@_camera_node\Translate(Vector3(1.0, 0.0, 0.0) * @MOVE_SPEED * time_step)

		if input\GetKeyDown(KEY_X)
			@_camera_node\Translate(Vector3(0.0, 0.0, 1.0) * @MOVE_SPEED * time_step)

		if input\GetKeyDown(KEY_Z)
			@_camera_node\Translate(Vector3(0.0, 0.0, -1.0) * @MOVE_SPEED * time_step)

	_on_key_down: (event_type, event_data)=>
		key = event_data["Key"]\GetInt()
		if key == KEY_ESCAPE
			engine\Exit()

		if key == KEY_SPACE
			input\SetMouseVisible(not input\IsMouseVisible())
			input\SetMouseGrabbed(not input\IsMouseVisible())

		for fn in *@_key_down_cbs
			fn(event_type, event_data)

return DebugScene

