--package.path = (
--	fileSystem:GetProgramDir() .. 'Data/Scripts/?.lua;' 
--	.. package.path 
--)

--require("Scripts/debugger")("127.0.0.1", 10001, "luaidekey")

function Init()
	Log:Write(LOG_INFO, "UrFelt Initialising")
		
	input:SetMouseVisible(true)
	input:SetMouseGrabbed(false)
	-- Get default style
	local uiStyle = cache:GetResource("XMLFile", "UI/DefaultStyle.xml")
	if uiStyle == nil then
		Log:Write(LOG_ERROR, "Can't find resource")
		return
	end
	
	-- Create console
	engine:CreateConsole()
	console.defaultStyle = uiStyle
	console.background.opacity = 0.8
	console:SetCommandInterpreter("LuaScript")
	
	-- Create debug HUD
	engine:CreateDebugHud()
	debugHud.defaultStyle = uiStyle

	-- Create scene.
	scene_ = Scene()

	-- Create the Octree component to the scene so that drawable objects can be 
	-- rendered. Use default volume
	-- (-1000, -1000, -1000) to (1000, 1000, 1000)
	scene_:CreateComponent("Octree")

	-- Create a Zone for ambient light & fog control
	local zoneNode = scene_:CreateChild("Zone")
	local zone = zoneNode:CreateComponent("Zone")
	zone.boundingBox = BoundingBox(-1000.0, 1000.0)
    zone.fogColor = Color(1.0, 1.0, 1.0)
    zone.fogStart = 300.0
    zone.fogEnd = 500.0
    zone.ambientColor = Color(0.15, 0.15, 0.15)

	-- Create the camera. Create it outside the scene so that we can clear the 
	-- whole scene without affecting it
	-- NOTE: can't create outside scene if attaching a point light to it.
--	cameraNode = Node()
	cameraNode = scene_:CreateChild("Camera")	
	cameraNode.position = Vector3(0.0, 10.0, -50.0)
	camera = cameraNode:CreateComponent("Camera")
	camera.farClip = 1000.0

	local viewport = Viewport:new(scene_, cameraNode:GetComponent("Camera"))
	renderer:SetViewport(0, viewport)   
 
	 -- Create a directional light
--	local lightNode = cameraNode:CreateChild("PointLight")
--	lightNode.position = Vector3(0.0, 0.0, 0.0)  
	 -- The direction vector does not need to be normalized
--	lightNode.direction = Vector3(-0.6, -1.0, 0.8)
    local lightNode = scene_:CreateChild("DirectionalLight")
    lightNode.direction = Vector3(0.6, -1.0, 0.8)
    local light = lightNode:CreateComponent("Light")
    light.lightType = LIGHT_DIRECTIONAL
    light.castShadows = true
    light.shadowBias = BiasParameters(0.00025, 0.5)
    -- Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light.shadowCascade = CascadeParameters(10.0, 50.0, 200.0, 0.0, 0.8)

	local point_light = cameraNode:CreateComponent("Light")
	point_light.lightType = LIGHT_POINT
	point_light.color = Color(1.0, 1.0, 1.0)
	point_light.specularIntensity = 0.001
	point_light.range = 50
	-- Set up a viewport to the Renderer subsystem so that the 3D scene can be 
	-- seen
	
    local skyNode = scene_:CreateChild("Sky")
    skyNode:SetScale(1000.0) -- The scale actually does not matter
    local skybox = skyNode:CreateComponent("Skybox")
    skybox.model = cache:GetResource("Model", "Models/Box.mdl")
    skybox.material = cache:GetResource("Material", "Materials/Skybox.xml")
	
	-- Construct new Text object, set string to display and font to use
	coord_ui_txt = ui.root:CreateChild("Text")
	coord_ui_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
	-- The text has multiple rows. Center them in relation to each other
	coord_ui_txt.textAlignment = HA_CENTER
	-- Position the text relative to the screen center
	coord_ui_txt.horizontalAlignment = HA_RIGHT
	coord_ui_txt.verticalAlignment = VA_BOTTOM
	coord_ui_txt:SetPosition(0, 0)	
	
	percent_top_ui_txt = ui.root:CreateChild("Text")
	percent_top_ui_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
	percent_top_ui_txt.textAlignment = HA_CENTER
	percent_top_ui_txt.horizontalAlignment = HA_CENTER
	percent_top_ui_txt.verticalAlignment = VA_TOP
	percent_top_ui_txt:SetPosition(0, 18)	
	
	percent_bottom_ui_txt = ui.root:CreateChild("Text")
	percent_bottom_ui_txt:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
	percent_bottom_ui_txt.textAlignment = HA_CENTER
	percent_bottom_ui_txt.horizontalAlignment = HA_CENTER
	percent_bottom_ui_txt.verticalAlignment = VA_BOTTOM
	percent_bottom_ui_txt:SetPosition(0, 0)	
		
	-- Subscribe key down event
	SubscribeToEvent("KeyDown", "HandleKeyDown")
	SubscribeToEvent("Update", "HandleUpdate")
	
end

function InitPhysics()
    -- Create a floor object, 1000 x 1000 world units. Adjust position so that the ground is at zero Y
    local floorNode = scene_:CreateChild("Floor")
    floorNode.position = Vector3(0.0, -100, 0.0)
    floorNode.scale = Vector3(1000.0, 1.0, 1000.0)
    local floorObject = floorNode:CreateComponent("StaticModel")
    floorObject.model = cache:GetResource("Model", "Models/Box.mdl")
    floorObject.material = cache:GetResource("Material", "Materials/StoneTiled.xml")

    -- Make the floor physical by adding RigidBody and CollisionShape components. The RigidBody's default
    -- parameters make the object static (zero mass.) Note that a CollisionShape by itself will not participate
    -- in the physics simulation
    local body = floorNode:CreateComponent("RigidBody")
    local shape = floorNode:CreateComponent("CollisionShape")
    -- Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene node scale, so the
    -- rendering and physics representation sizes should match (the box model is also 1 x 1 x 1.)
    shape:SetBox(Vector3(1.0, 1.0, 1.0))
end

function HandleKeyDown(eventType, eventData)
	local key = eventData["Key"]:GetInt()
	-- Close console (if open) or exit when ESC is pressed
	if key == KEY_ESC then
		if not console:IsVisible() then
			engine:Exit()
		else
			console:SetVisible(false)
		end

	elseif key == KEY_F1 then
		console:Toggle()

	elseif key == KEY_F2 then
		debugHud:ToggleAll()
	end
	
	if input:GetKeyDown(KEY_SPACE) then		
		input:SetMouseVisible(not input:IsMouseVisible())
		input:SetMouseGrabbed(not input:IsMouseGrabbed())
	end	
end


yaw = 0
pitch = 0
zapping = {amount=0}

main_initialised = false
worker_initialised = false

function HandleUpdate(eventType, eventData)
	-- Take the frame time step, which is stored as a float
	local timeStep = eventData["TimeStep"]:GetFloat()
 	
 	local msg = queue_in:pop()	
	while msg do
		if msg.type == "PERCENT_TOP" then 
			if msg.value < 0 then
				percent_top_ui_txt:SetText("")
			else
				percent_top_ui_txt:SetText("Loading physics " .. msg.value .. "%") 
			end
			
		elseif msg.type == "PERCENT_BOTTOM" then 
			if msg.value < 0 then
				percent_bottom_ui_txt:SetText("")
			else
				percent_bottom_ui_txt:SetText("Constructing surface " .. msg.value .. "%") 
			end
			
		elseif msg == "MAIN_INIT_DONE" then 
			main_initialised = true
			if worker_initialised then
				queue_main:push({type="STATE_RUNNING"})
				queue_worker:push({type="STATE_RUNNING"})
			end
				
		elseif msg == "WORKER_INIT_DONE" then 
			worker_initialised = true
			if main_initialised then
				queue_main:push({type="STATE_RUNNING"})
				queue_worker:push({type="STATE_RUNNING"})
			end
		end
		
		msg = queue_in:pop()
	end
	
	if not main_initialised or not worker_initialised then 
		return 
	end
	
	-- Do not move if the UI has a focused element (the console)
	if ui.focusElement ~= nil then
		return
	end
	
	if input:IsMouseVisible() and not input:IsMouseGrabbed() then
	   	local mousePos = input:GetMousePosition()
	   	local screenCoordX = mousePos.x / graphics:GetWidth()
	   	local screenCoordY = mousePos.y / graphics:GetHeight()
	   	local ray = camera:GetScreenRay(screenCoordX, screenCoordY)
	   	
		if input:GetMouseButtonDown(1) and zapping.amount <= 0 then
--		   	Log:Write(
--		   		LOG_INFO, "+'ve zap (" .. mousePos.x .. ", " .. mousePos.y ..") -> ("
--		   		..  screenCoordX .. ", " .. screenCoordY .. ")"
--		   	)
			zapping.amount = 1
			queue_worker:push({
				type="START_ZAP",
				ray=ray,
				amount=zapping.amount
			})
			
		elseif input:GetMouseButtonDown(4) and zapping.amount >= 0 then
			zapping.amount = -1
			queue_worker:push({
				type="START_ZAP",
				ray=ray,
				amount=zapping.amount
			})
			
		elseif zapping.amount ~= 0 then
			zapping.amount = 0
			queue_worker:push({
				type="STOP_ZAP"
			})
		end
	end	
	if input:GetKeyPress(KEY_R) then
		felt:repoly()
	end	 
	
	if not input:IsMouseGrabbed() then
		return
	end

	-- Movement speed as world units per second
	local MOVE_SPEED = 20.0
	-- Mouse sensitivity as degrees per pixel
	local MOUSE_SENSITIVITY = 0.1

	-- Use this frame's mouse motion to adjust camera node yaw and pitch. 
	-- Clamp the pitch between -90 and 90 degrees
	local mouseMove = input.mouseMove
	yaw = yaw + MOUSE_SENSITIVITY * mouseMove.x
	pitch = pitch - MOUSE_SENSITIVITY * mouseMove.y
	pitch = Clamp(pitch, -90.0, 90.0)

	-- Construct new orientation for the camera scene node from yaw and pitch. 
	-- Roll is fixed to zero
	cameraNode.rotation = Quaternion(pitch, yaw, 0.0)

	-- Read WASD keys and move the camera scene node to the corresponding direction if they are 
	-- pressed
	if input:GetKeyDown(KEY_W) then
		cameraNode:Translate(Vector3(0.0, 1.0, 0.0) * MOVE_SPEED * timeStep)
	end
	if input:GetKeyDown(KEY_S) then
		cameraNode:Translate(Vector3(0.0, -1.0, 0.0) * MOVE_SPEED * timeStep)
	end
	if input:GetKeyDown(KEY_A) then
		cameraNode:Translate(Vector3(-1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
	end
	if input:GetKeyDown(KEY_D) then
		cameraNode:Translate(Vector3(1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
	end
	if input:GetKeyDown(KEY_X) then
		cameraNode:Translate(Vector3(0.0, 0.0, 1.0) * MOVE_SPEED * timeStep)
	end
	if input:GetKeyDown(KEY_Z) then
		cameraNode:Translate(Vector3(0.0, 0.0, -1.0) * MOVE_SPEED * timeStep)
	end  
	
    if input:GetKeyPress(KEY_P) then
        SpawnObject()
    end	
	
	coord_ui_txt:SetText(
		"(" .. cameraNode:GetWorldPosition():ToString() 
		.. ") (" .. cameraNode:GetWorldRotation():ToString() .. ")"
	)
end


function SpawnObject()
    -- Create a smaller box at camera position
    local boxNode = scene_:CreateChild("SmallBox")
    boxNode.position = cameraNode.position
    boxNode.rotation = cameraNode.rotation
    boxNode:SetScale(1)
    local boxObject = boxNode:CreateComponent("StaticModel")
    boxObject.model = cache:GetResource("Model", "Models/Box.mdl")
    boxObject.material = cache:GetResource("Material", "Materials/StoneTiled.xml")
    boxObject.castShadows = true

    -- Create physics components, use a smaller mass also
    local body = boxNode:CreateComponent("RigidBody")
    body.mass = 0.25
    body.friction = 0.75
    body.restitution = 0
    local shape = boxNode:CreateComponent("CollisionShape")
    shape:SetBox(Vector3(1.0, 1.0, 1.0))

    local OBJECT_VELOCITY = 10.0

    -- Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
    -- to overcome gravity better
    body.linearVelocity = cameraNode.rotation * Vector3(0.0, 0.25, 1.0) * OBJECT_VELOCITY
end