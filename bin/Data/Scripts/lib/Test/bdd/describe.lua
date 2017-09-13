local testBuilder = require 'Test.Builder'.new()
local bdd = require 'Test.bdd'

local describe =
{
    mt = {},
    prototype = {}
}
describe.mt.__index = describe.prototype

local describeFn = function( subject )
    local instance =
    {
        subject = subject,
        behaviours = {},
        setupFn = nil,
        teardownFn = nil,
        beforeEach = nil,
        afterEach = nil
    }
    setmetatable(instance, describe.mt)
    bdd.addDescription(instance)
    return instance
end

function describe.prototype:setup( fn )
    self.setupFn = fn
    return self
end

function describe.prototype:teardown( fn )
    self.teardownFn = fn
    return self
end

function describe.prototype:beforeEach( fn )
    self.beforeEachFn = fn
    return self
end

function describe.prototype:afterEach( fn )
    self.afterEachFn = fn
    return self
end

function describe.prototype:it( description, testFn )
    local behaviour =
    {
        description = self.subject..' '..description,
        beforeFn = self.beforeEachFn,
        testFn = testFn,
        afterFn = self.afterEachFn
    }
    table.insert(self.behaviours, behaviour)
    return self
end

function describe.prototype:_runTests()
    local success = true
    if self.setupFn then self.setupFn() end
    for _,behaviour in ipairs(self.behaviours) do
        success = success and self:_verifyBehaviour(behaviour)
    end
    if self.teardownFn then self.teardownFn() end
    
    return success
end

local function messageHandler( originalMessage )
    return debug.traceback(originalMessage, 2)
end

function describe.prototype:_verifyBehaviour( behaviour )
    if behaviour.beforeFn then behaviour.beforeFn() end
    local success, message = xpcall(behaviour.testFn, messageHandler)
    if behaviour.afterFn then behaviour.afterFn() end

    if success then
        testBuilder:ok(true, behaviour.description, 999)
    else
        testBuilder:diag(message)
        testBuilder:ok(false, behaviour.description, 999)
    end
    
    return success
end

return describeFn