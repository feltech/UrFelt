local DebugScene = require('debug_scene')
local CollisionTest
do
  local _class_0
  local _parent_0 = DebugScene
  local _base_0 = {
    _on_update = function(self, event_type, event_data)
      _class_0.__parent.__base._on_update(self, event_type, event_data)
      local current_time = now()
      if current_time - self._last_poly > 30 then
        self._last_poly = now()
        self.surface:polygonise(function()
          return self.surface:flush()
        end)
      end
      if current_time - self._last_poll > 5 then
        return self.surface:poll()
      end
    end
  }
  _base_0.__index = _base_0
  setmetatable(_base_0, _parent_0.__base)
  _class_0 = setmetatable({
    __init = function(self)
      _class_0.__parent.__init(self)
      self._last_poly = now()
      self._last_poll = now()
      self:recreate()
      self._camera_node.position = Vector3(0, 20, -20)
      self._camera_node.rotation = Quaternion(0.9, 0.37, 0, 0)
      self._camera_node.rotation:Normalize()
      local floor_node = self.scene:CreateChild("Floor")
      floor_node.position = Vector3(0.0, -200, 0.0)
      floor_node.scale = Vector3(1000.0, 1.0, 1000.0)
      local floor_object = floor_node:CreateComponent("StaticModel")
      floor_object.model = cache:GetResource("Model", "Models/Box.mdl")
      floor_object.material = cache:GetResource("Material", "Materials/StoneTiled.xml")
      local body = floor_node:CreateComponent("RigidBody")
      local shape = floor_node:CreateComponent("CollisionShape")
      shape:SetBox(Vector3(1.0, 1.0, 1.0))
      self:log("Creating surface")
      local node = self.scene:CreateChild("Surface")
      self.surface = UrFelt.UrSurface(IntVector3(200, 200, 200), IntVector3(10, 10, 10), node)
      self:log("Creating box")
      self.surface:seed(IntVector3(0, 0, 0))
      self.surface:expand_by_constant(-1)
      return self.surface:transform_to_box(Vector3(-5, -5, -5), Vector3(5, 5, 5), function()
        local box_node = self.scene:CreateChild("SmallBox")
        box_node.position = Vector3(3, 8, -3)
        box_node:SetScale(1)
        local box_object = box_node:CreateComponent("StaticModel")
        box_object.model = cache:GetResource("Model", "Models/Box.mdl")
        box_object.material = cache:GetResource("Material", "Materials/StoneTiled.xml")
        box_object.castShadows = true
        body = box_node:CreateComponent("RigidBody")
        body.mass = 0.25
        body.friction = 0.75
        body.restitution = 0.1
        shape = box_node:CreateComponent("CollisionShape")
        return shape:SetBox(Vector3(1.0, 1.0, 1.0))
      end)
    end,
    __base = _base_0,
    __name = "CollisionTest",
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
  CollisionTest = _class_0
end
local scene = CollisionTest()
