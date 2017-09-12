test = require 'u-test'
mach = require 'mach'
assert = require 'luassert'
match = require 'luassert.match'
spy = require 'luassert.spy'
stub = require 'luassert.stub'
mock = require 'luassert.mock'
  
test.start_up = ()->
	print("start")

test.tear_down = ()->
	print("end")
	
test.mouse.start_up = ()->
	print("start mouse")

test.mouse.tear_down = ()->
	print("end mouse")
	
test.other.start_up = ()->
	print("start other")

test.other.tear_down = ()->
	print("end other")
	
test.mouse.concat = () ->
	test.is_userdata(input)

test.summary()