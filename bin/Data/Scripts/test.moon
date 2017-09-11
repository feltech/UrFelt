test = require 'u-test'

test.string.find = () ->
	test.is_nil(string.find("u-test", "banana"))
	test.is_not_nil(string.find("u-test", "u"))


-- You can declare test case with parameters
test.string.starts_with = (str, prefix) ->
	test.equal(string.find(str, prefix), 1)


test.string.starts_with("Lua rocks", "Lua")
test.string.starts_with("Wow", "Wow")

global_table = {}

-- Each test suite can be customized by start_up and tear_down
test.table.start_up = () ->
	global_table = { 1, 2, "three", 4, "five" }

test.table.tear_down = () -> 
	global_table = {}


test.table.concat = () ->
	test.equal(table.concat(global_table, ", "), "1, 2, three, 4, five")


test.summary()