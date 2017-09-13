test = require 'u-test'
mach = require 'mach'
assert = require 'luassert'
match = require 'luassert.match'
spy = require 'luassert.spy'
stub = require 'luassert.stub'
mock = require 'luassert.mock'

snapshot = nil

test.start_up = () ->
	snapshot = assert\snapshot()

test.tear_down = () ->
	snapshot\revert()
	
test.stub.start_up = ()->
	test.start_up()

test.stub.tear_down = ()->
	test.tear_down()
	
test.stub.create = () ->
	s = stub(getmetatable(input), "SetMouseVisible")
	
	input\SetMouseVisible(true)

	assert.is_not_nil(getmetatable(input.SetMouseVisible))
	assert.stub(s).was.called_with(input, true)
	
test.stub.torn_down = () ->
	assert.is_nil(getmetatable(input.SetMouseVisible))

test.summary()