local DebugScene = require('debug_scene')
local FSM = require('fsm')
local State, Idle, Load, Generate, Running
local BrainApp
do
  local _class_0
  local _parent_0 = DebugScene
  local _base_0 = {
    _transition = function(self, cls)
      self._state:tear_down()
      self._state = cls(self)
    end,
    _on_update = function(self, event_type, event_data)
      _class_0.__parent.__base._on_update(self, event_type, event_data)
      return self._state:poll()
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self)
      self.surface = nil
      self._state = Idle()
      _class_0.__parent.__init(self)
      self.fsm = FSM.create({
        initial = "idle",
        events = {
          {
            name = "load",
            from = "idle",
            to = "load_from_disk"
          },
          {
            name = "loaded",
            from = "load_from_disk",
            to = "generate"
          },
          {
            name = "generated",
            from = "generate",
            to = "running"
          },
          {
            name = "regenerate",
            from = "running",
            to = "generate"
          }
        },
        callbacks = {
          on_enter_idle = function(fsm, event, fro, to)
            return self:_transition(Idle)
          end,
          on_enter_load_from_disk = function(fsm, event, fro, to)
            return self:_transition(Load)
          end,
          on_enter_generate = function(fsm, event, fro, to, surface)
            return self:_transition(Generate)
          end,
          on_enter_running = function(fsm, event, fro, to)
            return self:_transition(Running)
          end
        }
      })
      self:recreate()
      local floor_node = self.scene:CreateChild("Floor")
      floor_node.position = Vector3(0.0, -200, 0.0)
      floor_node.scale = Vector3(1000.0, 1.0, 1000.0)
      local floor_object = floor_node:CreateComponent("StaticModel")
      floor_object.model = cache:GetResource("Model", "Models/Box.mdl")
      floor_object.material = cache:GetResource("Material", "Materials/StoneTiled.xml")
      local body = floor_node:CreateComponent("RigidBody")
      local shape = floor_node:CreateComponent("CollisionShape")
      return shape:SetBox(Vector3(1.0, 1.0, 1.0))
    end,
    __base = _base_0,
    __name = "BrainApp",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  BrainApp = _class_0
end
do
  local _class_0
  local _base_0 = {
    tear_down = function(self) end,
    poll = function(self) end
  }
  _base_0.__index = _base_0
  _class_0 = setmetatable({
    __init = function(self, app)
      print("Entering " .. self.__class.__name)
      self._app = app
    end,
    __base = _base_0,
    __name = "State"
  }, {
    __index = _base_0,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  State = _class_0
end
local LoadingBase
do
  local _class_0
  local _parent_0 = State
  local _base_0 = {
    tear_down = function(self)
      return self._ui_txt:Remove()
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self, app)
      _class_0.__parent.__init(self, app)
      self._ui_txt = ui.root:CreateChild("Text")
      self._ui_txt.textAlignment = HA_CENTER
      self._ui_txt.horizontalAlignment = HA_CENTER
      self._ui_txt.verticalAlignment = VA_CENTER
      return self._ui_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 20)
    end,
    __base = _base_0,
    __name = "LoadingBase",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  LoadingBase = _class_0
end
do
  local _class_0
  local _parent_0 = LoadingBase
  local _base_0 = {
    poll = function(self)
      _class_0.__parent.__base.poll(self)
      if input:GetKeyPress(KEY_RETURN) then
        self._app.fsm.load()
        return 
      end
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self, app)
      _class_0.__parent.__init(self, app)
      return self._ui_txt:SetText("Prese ENTER to start")
    end,
    __base = _base_0,
    __name = "Idle",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  Idle = _class_0
end
do
  local _class_0
  local _parent_0 = LoadingBase
  local _base_0 = {
    poll = function(self)
      _class_0.__parent.__base.poll(self)
      if self._loader:ready() then
        print("Surface loaded")
        self._app.surface = self._loader:get()
        return self._app.fsm.loaded()
      end
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self, app, brain_scene)
      _class_0.__parent.__init(self, app)
      print("Creating Surface node")
      local node = self._app.scene:CreateChild("Surface")
      local filepath = GetFileSystem().GetProgramDir() .. "Data/brain.bin.gz"
      print("Loading surface from " .. filepath)
      self._ui_txt:SetText("Loading from disk...")
      self._loader = UrFelt.UrSurface.load(filepath, node)
    end,
    __base = _base_0,
    __name = "Load",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  Load = _class_0
end
do
  local _class_0
  local _parent_0 = LoadingBase
  local _base_0 = {
    poll = function(self)
      _class_0.__parent.__base.poll(self)
      self._app.surface:poll()
      if self._flushed then
        return self._app.fsm.generated()
      end
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self, app, surface)
      _class_0.__parent.__init(self, app)
      self._ui_txt:SetText("Regenerating...")
      self._flushed = false
      self._app.surface:invalidate()
      print("Updating polygonisation...")
      return self._app.surface:polygonise(function()
        print("Updating physics shapes...")
        self._app.surface:flush()
        self._flushed = true
      end)
    end,
    __base = _base_0,
    __name = "Generate",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  Generate = _class_0
end
do
  local _class_0
  local _parent_0 = State
  local _base_0 = {
    OBJECT_VELOCITY = 20,
    poll = function(self)
      if input:GetKeyPress(KEY_R) then
        self._app.fsm.regenerate()
        return 
      end
      if input:IsMouseGrabbed() then
        if input:GetMouseButtonPress(MOUSEB_LEFT) then
          self:_throw_box()
        end
      elseif input:IsMouseVisible() then
        if self._op_start_time == nil then
          if input:GetMouseButtonDown(MOUSEB_LEFT) then
            self:_lower_surface()
          else
            if input:GetMouseButtonDown(MOUSEB_RIGHT) then
              self:_raise_surface()
            end
          end
        end
      end
      local current_time = now()
      if current_time - self._last_poly > 30 then
        self._last_poly = now()
        self._app.surface:polygonise(function()
          return self._app.surface:flush()
        end)
      end
      if current_time - self._last_poll > 5 then
        self._app.surface:poll()
      end
      if self._op_start_time ~= nil then
        if current_time - self._op_start_time > 50 then
          return self._op:stop()
        end
      end
    end,
    _raise_surface = function(self, direction)
      local pos_hit = self:_raycast()
      if pos_hit == nil then
        return 
      end
      self._op_start_time = now()
      self._op = self._app.surface:attract_to_sphere(pos_hit, 3, function()
        self._op_start_time = nil
      end)
    end,
    _lower_surface = function(self, direction)
      local pos_hit = self:_raycast()
      if pos_hit == nil then
        return 
      end
      self._op_start_time = now()
      self._op = self._app.surface:repel_from_sphere(pos_hit, 3, function()
        self._op_start_time = nil
      end)
    end,
    _raycast = function(self)
      local mouse_pos = input:GetMousePosition()
      local screen_coordX = mouse_pos.x / graphics:GetWidth()
      local screen_coordY = mouse_pos.y / graphics:GetHeight()
      if screen_coordX < 0 or screen_coordX > 1 or screen_coordY < 0 or screen_coordY > 1 then
        print("Outside bounds (" .. screen_coordX .. ", " .. screen_coordY .. ")")
        return 
      end
      local ray = self._app.camera:GetScreenRay(screen_coordX, screen_coordY)
      local pos_hit = self._app.surface:ray(ray)
      return pos_hit
    end,
    _throw_box = function(self)
      local camera_node = self._app.camera:GetNode()
      local box_node = self._app.scene:CreateChild("SmallBox")
      box_node.position = camera_node.position
      box_node.rotation = camera_node.rotation
      box_node:SetScale(1)
      local box_object = box_node:CreateComponent("StaticModel")
      box_object.model = cache:GetResource("Model", "Models/Box.mdl")
      box_object.material = cache:GetResource("Material", "Materials/StoneTiled.xml")
      box_object.castShadows = true
      local body = box_node:CreateComponent("RigidBody")
      body.mass = 0.25
      body.friction = 0.75
      body.restitution = 0
      local shape = box_node:CreateComponent("CollisionShape")
      shape:SetBox(Vector3(1.0, 1.0, 1.0))
      body.linearVelocity = camera_node.rotation * Vector3(0.0, 0.25, 1.0) * self.OBJECT_VELOCITY
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self, app)
      _class_0.__parent.__init(self, app)
      self._last_poly = now()
      self._last_poll = now()
      self._op = nil
      self._op_start_time = nil
    end,
    __base = _base_0,
    __name = "Running",
    __parent = _parent_0
  }, {
    __index = function(cls, name)
      local val = rawget(_base_0, name)
      if val == nil then
        local parent = rawget(cls, "__parent")
        if parent then
          return parent[name]
        end
      else
        return val
      end
    end,
    __call = function(cls, ...)
      local _self_0 = setmetatable({}, _base_0)
      cls.__init(_self_0, ...)
      return _self_0
    end
  })
  _base_0.__class = _class_0
  if _parent_0.__inherited then
    _parent_0.__inherited(_parent_0, _class_0)
  end
  Running = _class_0
end
local scene = BrainApp()
