local test = require('u-test')
local mach = require('mach')
local assert = require('luassert')
local match = require('luassert.match')
local spy = require('luassert.spy')
local stub = require('luassert.stub')
local mock = require('luassert.mock')
test.start_up = function()
  return print("start")
end
test.tear_down = function()
  return print("end")
end
test.mouse.start_up = function()
  return print("start mouse")
end
test.mouse.tear_down = function()
  return print("end mouse")
end
test.other.start_up = function()
  return print("start other")
end
test.other.tear_down = function()
  return print("end other")
end
test.mouse.concat = function()
  test.is_userdata(input)
  mock(input, true)
  test.is_userdata(input)
  return assert.stub((function()
    local _base_0 = input
    local _fn_0 = _base_0.SetMouseVisible
    return function(...)
      return _fn_0(_base_0, ...)
    end
  end)()).was.called_with(true)
end
return test.summary()
