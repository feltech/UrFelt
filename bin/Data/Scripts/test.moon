assert = require 'luassert'
stub = require 'luassert.stub'
bdd = require 'Test.bdd'
describe = require 'Test.bdd.describe'

snapshot = nil

print("v2")

describe(
	'stubbing userdata'
		
)\beforeEach( () ->
	snapshot = assert\snapshot()
	
)\afterEach( () ->
	snapshot\revert()
	
)\it('successfully stubs userdata', () ->
	
	s = stub(getmetatable(input), "SetMouseVisible")
	
	input\SetMouseVisible(true)

	assert.is_not_nil(getmetatable(input.SetMouseVisible))
	assert.stub(s).was.called_with(input, true)
		
)\it('removes the stub when done', () ->
	
	assert.is_nil(getmetatable(input.SetMouseVisible))
)

success = bdd.runTests()
os.exit(success and 0 or 1)





