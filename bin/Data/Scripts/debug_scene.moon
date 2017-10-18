bootstrap_scene = ()->
	scene = Scene()
	scene\CreateComponent("Octree")

	camera_node = scene\CreateChild("Camera")
	camera_node.position = Vector3(0.0, 0.0, -50.0)
	camera = camera_node\CreateComponent("Camera")
	viewport = Viewport\new(scene, camera_node\GetComponent("Camera"))
	renderer\SetViewport(0, viewport)

	light_node = scene\CreateChild("DirectionalLight")
	light_node.direction = Vector3(0.6, -1.0, 0.8)
	light = light_node\CreateComponent("Light")
	light.lightType = LIGHT_DIRECTIONAL

	point_light = camera_node\CreateComponent("Light")
	point_light.lightType = LIGHT_POINT
	point_light.color = Color(1.0, 1.0, 1.0)
	point_light.specularIntensity = 0.001
	point_light.range = 50

	return scene, camera


export updateFPSDisplay = (eventType, eventData)->
	timeStep = eventData["TimeStep"]\GetFloat()
	ui_fps_txt\SetText("FPS: " .. tostring(1/timeStep))


export updateCameraMovement = ()->
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


export handleKeyDown = (eventType, eventData)->
	key = eventData["Key"]\GetInt()
	-- Close console (if open) or exit when ESC is pressed
	if key == KEY_ESC
		engine\Exit()

	if input\GetKeyDown(KEY_SPACE)
		input\SetMouseVisible(not input\IsMouseVisible())
		input\SetMouseGrabbed(not input\IsMouseVisible())


-- export MOVE_SPEED
-- export MOUSE_SENSITIVITY
-- export yaw
-- export pitch
-- export ui_fps_txt

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

SubscribeToEvent("Update", "updateFPSDisplay")
SubscribeToEvent("Update", "updateCameraMovement")
SubscribeToEvent("KeyDown", "handleKeyDown")


return bootstrap_scene
