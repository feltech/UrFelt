local DebugScene = require('debug_scene')
local surface = nil
local debug_scene = DebugScene()
debug_scene:recreate()
local node = debug_scene.scene:CreateChild("Surface")
surface = UrFelt.UrSurface(IntVector3(200, 200, 200), IntVector3(10, 10, 10), node)
surface:seed(IntVector3(40, 0, 30))
surface:seed(IntVector3(-40, 0, 30))
surface:seed(IntVector3(40, 0, -30))
surface:seed(IntVector3(-40, 0, -30))
surface:expand_by_constant(-1)
local last_render = now()
local last_poll = last_render
local is_rendering = false
local saved = false
local started = false
local ui_txt = ui.root:CreateChild("Text")
ui_txt.textAlignment = HA_CENTER
ui_txt.horizontalAlignment = HA_CENTER
ui_txt.verticalAlignment = VA_CENTER
ui_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 20)
ui_txt:SetText("Prese ENTER to start")
local on_update
on_update = function(eventType, eventData)
  if surface ~= nil then
    if now() - last_poll > 1000 / 20 then
      last_poll = now()
      surface:poll()
    end
    if now() - last_render > 1000 / 10 then
      last_render = now()
      if not is_rendering then
        is_rendering = true
        surface:polygonise(function()
          surface:flush_graphics()
          is_rendering = false
        end)
      end
    end
  end
  if saved then
    return engine:Exit()
  end
end
local on_key
on_key = function(eventType, eventData)
  local key = eventData["Key"]:GetInt()
  if key == KEY_RETURN then
    if not started then
      local filepath = GetFileSystem().GetProgramDir() .. "/Data/brain.hdr"
      surface:transform_to_image(filepath, 0.58, 0.2, 0.2)
      started = true
      ui_txt:SetText("")
      ui_txt = nil
    else
      print("Saving ...")
      return surface:save("Data/brain.bin.gz", function()
        return print("... saved")
      end)
    end
  end
end
debug_scene:subscribe_to_update(on_update)
return debug_scene:subscribe_to_key_down(on_key)
