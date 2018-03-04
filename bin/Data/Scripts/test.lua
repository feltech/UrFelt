local lassert = require('luassert')
local stub = require('luassert.stub')
local DebugScene = require('debug_scene')
local Runner = require('feltest')
Runner.TIMEOUT = 120
local run = Runner()
local debug_scene = DebugScene()
local await_finish = nil
local snapshot = nil
run:describe('stubbing userdata'):beforeEach(function()
  snapshot = lassert:snapshot()
end):afterEach(function()
  return snapshot:revert()
end):it('successfully stubs userdata', function()
  local s = stub(getmetatable(input), "SetMouseVisible")
  input:SetMouseVisible(true)
  lassert.is_not_nil(getmetatable(input.SetMouseVisible))
  return lassert.stub(s).was.called_with(input, true)
end):it('removes the stub when done', function()
  return lassert.is_nil(getmetatable(input.SetMouseVisible))
end)
run:describe("surface", function(self)
  self:beforeEach(function(self)
    return debug_scene:recreate()
  end)
  self:it("can be constructed", function(self)
    local node = debug_scene.scene:CreateChild("Surface")
    self.surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
    return lassert.are.equal(type(self.surface), "userdata", "surface should be userdata")
  end)
  self:it("can spawn a seed", function(self)
    local node = debug_scene.scene:CreateChild("Surface")
    self.surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)
    return self.surface:seed(IntVector3(0, 0, 0))
  end)
  self:describe("ray", function(self)
    self:beforeEach(function(self)
      local node = debug_scene.scene:CreateChild("Surface")
      self.surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
      self.surface:seed(IntVector3(0, 0, 0))
      self.surface:expand_by_constant(-1)
      return self.surface:await()
    end)
    return self:it("can cast to surface", function(self)
      local ray = debug_scene.camera:GetScreenRay(0.5, 0.5)
      local pos_hit = self.surface:ray(ray)
      return lassert.is_equal(pos_hit, Vector3(0, 0, -1), Vector3(pos_hit):ToString() .. " != " .. Vector3(0, 0, 0):ToString())
    end)
  end)
  self:describe("async", function(self)
    self:beforeEach(function(self)
      local node = debug_scene.scene:CreateChild("Surface")
      self.surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
      return self.surface:seed(IntVector3(0, 0, 0))
    end)
    self:it("blocks on await", function(self)
      local finished = false
      local transformed = false
      self.surface:expand_by_constant(-1)
      self.surface:expand_by_constant(-1, function()
        transformed = true
      end)
      self.surface:polygonise(function()
        self.surface:flush()
        finished = true
      end)
      lassert.is_false(transformed)
      lassert.is_false(finished)
      coroutine.yield()
      lassert.is_false(finished)
      lassert.is_false(transformed)
      self.surface:await()
      lassert.is_true(finished)
      return lassert.is_true(transformed)
    end)
    return self:it("doesn't block on poll", function(self)
      local finished = false
      self.surface:expand_by_constant(-5, function()
        return self.surface:polygonise(function()
          self.surface:flush()
          finished = true
        end)
      end)
      lassert.is_false(finished)
      self.surface:poll()
      coroutine.yield()
      lassert.is_false(finished)
      local count = 0
      while not finished do
        count = count + 1
        self.surface:poll()
        coroutine.yield()
      end
      lassert.is_true(count > 1)
      return print("Iterations " .. tostring(count))
    end)
  end)
  self:describe("ops", function(self)
    self:beforeEach(function(self)
      self.finished = false
      self.await_finish = await_finish
      local node = debug_scene.scene:CreateChild("Surface")
      self.surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
      self.surface:seed(IntVector3(0, 0, 0))
      return self.surface:expand_by_constant(-1)
    end)
    self:describe("transform by constant", function(self)
      return self:it("can transform a small region", function(self)
        self.surface:expand_by_constant(Vector3(-1, -1, 1), Vector3(1, 1, 1), -1.0, function()
          return self.surface:polygonise(function()
            self.surface:flush()
            self.finished = true
          end)
        end)
        self:await_finish()
        local pos_hit = self.surface:ray(debug_scene.camera:GetScreenRay(0.5, 0.5))
        return lassert.is_equal(pos_hit, Vector3(0, 0, -1))
      end)
    end)
    self:describe("transform to box", function(self)
      self:it("can fill a box", function(self)
        self.surface:transform_to_box(Vector3(-10, -5, -10), Vector3(10, 5, 10), function()
          return self.surface:polygonise(function()
            self.surface:flush()
            self.finished = true
          end)
        end)
        return self:await_finish()
      end)
      return self:it("can move a box to fill a different box", function(self)
        self.surface:transform_to_box(Vector3(3, 3, 3), Vector3(11, 11, 11), function()
          return self.surface:transform_to_box(Vector3(-10, -10, -10), Vector3(0, 3, 0), function()
            return self.surface:polygonise(function()
              self.surface:flush()
              self.finished = true
            end)
          end)
        end)
        return self:await_finish()
      end)
    end)
    self:describe("attract to sphere", function(self)
      return self:it("can expand towards a sphere", function(self)
        self.surface:transform_to_box(Vector3(-5, -5, -5), Vector3(5, 5, 5), function()
          return self.surface:attract_to_sphere(Vector3(1, 1, -5), 3, function()
            return self.surface:polygonise(function()
              self.surface:flush()
              self.finished = true
            end)
          end)
        end)
        return self:await_finish()
      end)
    end)
    return self:describe("repel from sphere", function(self)
      return self:it("can contract towards a sphere", function(self)
        self.surface:transform_to_box(Vector3(-5, -5, -5), Vector3(5, 5, 5), function()
          return self.surface:repel_from_sphere(Vector3(1, 1, -5), 3, function()
            return self.surface:polygonise(function()
              self.surface:flush()
              self.finished = true
            end)
          end)
        end)
        return self:await_finish()
      end)
    end)
  end)
  self:describe("serialisation", function(self)
    self:beforeEach(function(self)
      self.node = debug_scene.scene:CreateChild("Surface")
      self.finished = false
      self.await_finish = await_finish
    end)
    self:it("can save to disk", function(self)
      self.surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), self.node)
      self.surface:seed(IntVector3(0, 0, 0))
      self.surface:expand_by_constant(-1)
      self.surface:transform_to_box(Vector3(-2, -2, -2), Vector3(2, 2, 2), function()
        return self.surface:save("/tmp/urfelt.test.bin", function()
          self.finished = true
        end)
      end)
      return self:await_finish()
    end)
    return self:it("can load from disk", function(self)
      local loader = UrFelt.UrSurface.load("/tmp/urfelt.test.bin", self.node)
      while not loader:ready() do
        io.write(".")
        coroutine.yield()
      end
      io.write("\n")
      self.surface = loader:get()
      self.surface:invalidate()
      self.surface:polygonise()
      self.surface:await()
      return self.surface:flush()
    end)
  end)
  return self:describe("image segmentation", function(self)
    return self:it('transforms to fit image until cancelled', function(self)
      local finished = false
      local start_time = now()
      local node = debug_scene.scene:CreateChild("Surface")
      self.surface = UrFelt.UrSurface(IntVector3(200, 200, 200), IntVector3(20, 20, 20), node)
      self.surface:seed(IntVector3(50, 50, 50))
      self.surface:seed(IntVector3(-50, -50, -50))
      self.surface:seed(IntVector3(50, 50, -50))
      self.surface:expand_by_constant(-1)
      local op = self.surface:transform_to_image("brain.hdr", 0.58, 0.2, 0.2, function()
        finished = true
      end)
      local last = now()
      local is_rendering = false
      local count = 0
      while not finished do
        count = count + 1
        if now() - last > 100 then
          if not is_rendering then
            is_rendering = true
            self.surface:polygonise(function()
              last = now()
              self.surface:flush_graphics()
              is_rendering = false
            end)
          end
          self.surface:poll()
        end
        if now() - start_time > 1000 then
          op:stop()
        end
        coroutine.yield()
      end
      return print("Segmentation took " .. tostring(now() - start_time) .. " ms")
    end)
  end)
end)
await_finish = function(self)
  local last = now()
  local count = 0
  local is_rendering = false
  while not self.finished do
    count = count + 1
    if now() - last > 100 then
      if not is_rendering then
        is_rendering = true
        self.surface:polygonise(function()
          last = now()
          self.surface:flush()
          is_rendering = false
        end)
      end
      self.surface:poll()
    end
    coroutine.yield()
  end
  local has_final_render = false
  self.surface:polygonise(function()
    self.surface:flush()
    has_final_render = true
  end)
  while not has_final_render do
    self.surface:poll()
    coroutine.yield()
  end
  return print("Iterations " .. tostring(count))
end
local resumeTesting
resumeTesting = function()
  local success = run:resumeTests()
  if success ~= nil then
    print("Tests completed with " .. (function()
      if success then
        return "success"
      else
        return "failure"
      end
    end)())
    return os.exit(sucesss and 0 or 1)
  end
end
debug_scene:subscribe_to_update(resumeTesting)
return run:runTests()
