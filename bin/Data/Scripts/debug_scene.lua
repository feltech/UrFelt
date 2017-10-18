local camera_node = nil
local ui_fps_txt = nil
local bootstrap_scene
bootstrap_scene = function()
  local scene = Scene()
  scene:CreateComponent("Octree")
  camera_node = scene:CreateChild("Camera")
  camera_node.position = Vector3(0.0, 0.0, -50.0)
  local camera = camera_node:CreateComponent("Camera")
  local viewport = Viewport:new(scene, camera_node:GetComponent("Camera"))
  renderer:SetViewport(0, viewport)
  local light_node = scene:CreateChild("DirectionalLight")
  light_node.direction = Vector3(0.6, -1.0, 0.8)
  local light = light_node:CreateComponent("Light")
  light.lightType = LIGHT_DIRECTIONAL
  local point_light = camera_node:CreateComponent("Light")
  point_light.lightType = LIGHT_POINT
  point_light.color = Color(1.0, 1.0, 1.0)
  point_light.specularIntensity = 0.001
  point_light.range = 50
  return scene, camera
end
updateFPSDisplay = function(eventType, eventData)
  print("Update display")
  local timeStep = eventData["TimeStep"]:GetFloat()
  return ui_fps_txt:SetText("FPS: " .. tostring(1 / timeStep))
end
local MOVE_SPEED = 30.0
local MOUSE_SENSITIVITY = 0.1
local yaw = 0
local pitch = 0
updateCameraMovement = function()
  print("Update camera")
  if input:IsMouseGrabbed() then
    local mouseMove = input.mouseMove
    yaw = yaw + MOUSE_SENSITIVITY * mouseMove.x
    pitch = pitch - MOUSE_SENSITIVITY * mouseMove.y
    camera_node.rotation = Quaternion(pitch, yaw, 0.0)
  end
  if input:GetKeyDown(KEY_W) then
    camera_node:Translate(Vector3(0.0, 1.0, 0.0) * MOVE_SPEED * timeStep)
  end
  if input:GetKeyDown(KEY_S) then
    camera_node:Translate(Vector3(0.0, -1.0, 0.0) * MOVE_SPEED * timeStep)
  end
  if input:GetKeyDown(KEY_A) then
    camera_node:Translate(Vector3(-1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
  end
  if input:GetKeyDown(KEY_D) then
    camera_node:Translate(Vector3(1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
  end
  if input:GetKeyDown(KEY_X) then
    camera_node:Translate(Vector3(0.0, 0.0, 1.0) * MOVE_SPEED * timeStep)
  end
  if input:GetKeyDown(KEY_Z) then
    return camera_node:Translate(Vector3(0.0, 0.0, -1.0) * MOVE_SPEED * timeStep)
  end
end
handleKeyDown = function(eventType, eventData)
  print("Pressed key")
  local key = eventData["Key"]:GetInt()
  if key == KEY_ESC then
    engine:Exit()
  end
  if key == KEY_SPACE then
    input:SetMouseVisible(not input:IsMouseVisible())
    return input:SetMouseGrabbed(not input:IsMouseVisible())
  end
end
input:SetMouseVisible(true)
input:SetMouseGrabbed(false)
ui_fps_txt = ui.root:CreateChild("Text")
ui_fps_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
ui_fps_txt.textAlignment = HA_CENTER
ui_fps_txt.horizontalAlignment = HA_LEFT
ui_fps_txt.verticalAlignment = VA_TOP
SubscribeToEvent("Update", "updateFPSDisplay")
SubscribeToEvent("Update", "updateCameraMovement")
SubscribeToEvent("KeyDown", "handleKeyDown")
return bootstrap_scene
