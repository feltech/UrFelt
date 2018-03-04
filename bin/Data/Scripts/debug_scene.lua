local SHOW_FPS_AND_POS = true
local DebugScene
do
  local _class_0
  local _base_0 = {
    MOVE_SPEED = 30.0,
    MOUSE_SENSITIVITY = 0.1,
    recreate = function(self)
      self.scene = Scene()
      self.scene:CreateComponent("Octree")
      self._camera_node = self.scene:CreateChild("Camera")
      self._camera_node.position = Vector3(-70.0, 180.0, -70.0)
      self._camera_node.rotation = Quaternion(0.8, 0.57, 0.14, 0.09)
      self._camera_node.rotation:Normalize()
      self.camera = self._camera_node:CreateComponent("Camera")
      local viewport = Viewport:new(self.scene, self._camera_node:GetComponent("Camera"))
      renderer:SetViewport(0, viewport)
      local light_node = self.scene:CreateChild("DirectionalLight")
      light_node.direction = Vector3(0.6, -1.0, 0.8)
      local light = light_node:CreateComponent("Light")
      light.lightType = LIGHT_DIRECTIONAL
      light.castShadows = true
      light.shadowBias = BiasParameters(0.00025, 0.5)
      light.shadowCascade = CascadeParameters(10.0, 50.0, 200.0, 0.0, 0.8)
      light.specularIntensity = 0.5
      local point_light = self._camera_node:CreateComponent("Light")
      point_light.lightType = LIGHT_POINT
      point_light.color = Color(1.0, 1.0, 1.0)
      point_light.specularIntensity = 0.001
      point_light.range = 100
    end,
    subscribe_to_update = function(self, fn)
      return table.insert(self._update_cbs, fn)
    end,
    subscribe_to_key_down = function(self, fn)
      return table.insert(self._key_down_cbs, fn)
    end,
    _on_update = function(self, event_type, event_data)
      local time_step = event_data["TimeStep"]:GetFloat()
      self:_update_FPS_display(time_step)
      self:_update_camera_movement(time_step)
      local _list_0 = self._update_cbs
      for _index_0 = 1, #_list_0 do
        local fn = _list_0[_index_0]
        fn(event_type, event_data)
      end
    end,
    _update_FPS_display = function(self, time_step)
      if SHOW_FPS_AND_POS then
        return self._ui_fps_txt:SetText(self._camera_node.position:ToString() .. " :: " .. self._camera_node.rotation:ToString() .. " :: " .. "FPS: " .. string.format("%.2f", 1 / time_step))
      end
    end,
    _update_camera_movement = function(self, time_step)
      if input:IsMouseGrabbed() then
        local mouseMove = input.mouseMove
        local yaw = self.MOUSE_SENSITIVITY * mouseMove.x
        local pitch = -self.MOUSE_SENSITIVITY * mouseMove.y
        self._camera_node.rotation = self._camera_node.rotation * Quaternion(pitch, yaw, 0)
        self._camera_node.rotation:Normalize()
      end
      if input:GetKeyDown(KEY_W) then
        self._camera_node:Translate(Vector3(0.0, 1.0, 0.0) * self.MOVE_SPEED * time_step)
      end
      if input:GetKeyDown(KEY_S) then
        self._camera_node:Translate(Vector3(0.0, -1.0, 0.0) * self.MOVE_SPEED * time_step)
      end
      if input:GetKeyDown(KEY_A) then
        self._camera_node:Translate(Vector3(-1.0, 0.0, 0.0) * self.MOVE_SPEED * time_step)
      end
      if input:GetKeyDown(KEY_D) then
        self._camera_node:Translate(Vector3(1.0, 0.0, 0.0) * self.MOVE_SPEED * time_step)
      end
      if input:GetKeyDown(KEY_X) then
        self._camera_node:Translate(Vector3(0.0, 0.0, 1.0) * self.MOVE_SPEED * time_step)
      end
      if input:GetKeyDown(KEY_Z) then
        return self._camera_node:Translate(Vector3(0.0, 0.0, -1.0) * self.MOVE_SPEED * time_step)
      end
    end,
    _on_key_down = function(self, event_type, event_data)
      local key = event_data["Key"]:GetInt()
      if key == KEY_ESCAPE then
        engine:Exit()
      end
      if key == KEY_SPACE then
        input:SetMouseVisible(not input:IsMouseVisible())
        input:SetMouseGrabbed(not input:IsMouseVisible())
      end
      local _list_0 = self._key_down_cbs
      for _index_0 = 1, #_list_0 do
        local fn = _list_0[_index_0]
        fn(event_type, event_data)
      end
    end
  }
  _base_0.__index = _base_0
  _class_0 = setmetatable({
    __init = function(self)
      self._ui_fps_txt = ui.root:CreateChild("Text")
      self._ui_fps_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
      self._ui_fps_txt.textAlignment = HA_CENTER
      self._ui_fps_txt.horizontalAlignment = HA_LEFT
      self._ui_fps_txt.verticalAlignment = VA_TOP
      self._update_cbs = { }
      self._key_down_cbs = { }
      input:SetMouseVisible(true)
      input:SetMouseGrabbed(false)
      SubscribeToEvent("Update", function(event_type, event_data)
        return self:_on_update(event_type, event_data)
      end)
      return SubscribeToEvent("KeyDown", function(event_type, event_data)
        return self:_on_key_down(event_type, event_data)
      end)
    end,
    __base = _base_0,
    __name = "DebugScene"
  }, {
    __index = _base_0,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  DebugScene = _class_0
end
return DebugScene
