Runner = nil

class Describe
	new: (runner, subject, fn, beforeFns={}, afterFns={}) =>
		@_runner = runner
		@_subject = subject
		@_priority_behaviours = {}
		@_behaviours = {}
		@_beforeFns = beforeFns
		@_afterFns = afterFns

		table.insert(@_runner.descriptions, self)

		if fn
			fn(self)

	describe: (subject, fn)=>
		desc = Describe(
			@_runner, @_subject .. " " .. subject, fn,
			[f for f in *@_beforeFns], [f for f in *@_afterFns]
		)
		return desc

	beforeEach: (fn)=>
		table.insert(@_beforeFns, fn)
		return self

	afterEach: (fn)=>
		table.insert(@_afterFns, fn)
		@_afterFn = fn
		return self

	it: (description, fn)=>
		table.insert(@_behaviours, {
			description: @_subject..' '..description,
			testFn: coroutine.create(fn),
		})
		return self

	fit: (description, fn)=>
		@_runner.priority_mode = true
		table.insert(@_priority_behaviours, {
			description: @_subject..' '..description,
			testFn: coroutine.create(fn),
		})
		return self

	_runTests: ()=>
		return coroutine.create(()->
			Runner.debug("describe._runTests: looping over tests")

			if @_runner.priority_mode
				@_behaviours = @_priority_behaviours

			for behaviour in *@_behaviours
				Runner.debug("describe._runTests: constructing coroutine for test")
				verify = @_verifyBehaviour(behaviour)

				while true
					Runner.debug("describe._runTests: performing next iteration of test")
					success, message = coroutine.resume(verify)
					if not success then
						Runner.debug("describe._runTests: test errored")
						error("\n" .. message)
					elseif coroutine.status(verify) == "suspended" then
						Runner.debug("describe._runTests: test incomplete")
						coroutine.yield()
					else
						Runner.debug("describe._runTests: test complete")
						break

			if @_teardownFn
				@_teardownFn()
		)

	_verifyBehaviour: (behaviour)=>
		return coroutine.create(()->
			start = os.time()
			Runner.debug("describe._verifyBehaviour: before x" .. tostring(#@_beforeFns))
			@_runner.curr_test_num += 1
			io.write("[" .. @_runner.curr_test_num .. "] " .. behaviour.description .. " ...\n")

			beforeFns = [coroutine.create(fn) for fn in *@_beforeFns]
			afterFns = [coroutine.create(fn) for fn in *@_afterFns]
			for beforeFn in *beforeFns
				start = os.time()
				Runner.debug("describe._verifyBehaviour: _beforeFn starting")
				while true
					is_ok, err = coroutine.resume(beforeFn, behaviour)
					if not is_ok
						Runner.debug("describe._verifyBehaviour: _beforeFn failed")
						error("\n" .. err)
					elseif coroutine.status(beforeFn) ~= "suspended"
						Runner.debug("describe._verifyBehaviour: _beforeFn done")
						break
					elseif os.time() - start > Runner.TIMEOUT
						error("timeout after " .. tostring(Runner.TIMEOUT) .. "s")
					else
						Runner.debug("describe._verifyBehaviour: _beforeFn incomplete")
						coroutine.yield()


			start = os.time()
			success = false
			message = nil
			while true
				Runner.debug("describe._verifyBehaviour: resuming actual test")
				success, message = coroutine.resume(behaviour.testFn, behaviour)
				if not success
					Runner.debug("describe._verifyBehaviour: test failed")
					break
				elseif coroutine.status(behaviour.testFn) ~= "suspended"
					break
				elseif os.time() - start > Runner.TIMEOUT
					success = false
					message = "timeout after " .. tostring(Runner.TIMEOUT) .. "s"
					break
				else
					Runner.debug("describe._verifyBehaviour: test incomplete")
					coroutine.yield()


			Runner.debug("describe._verifyBehaviour: after x" .. tostring(#@_afterFns))
			for afterFn in *afterFns
				start = os.time()
				Runner.debug("describe._verifyBehaviour: afterFn starting")
				while true
					is_ok, err = coroutine.resume(afterFn, behaviour)
					if not is_ok
						Runner.debug("describe._verifyBehaviour: afterFn failed")
						error("\n" .. err)
					elseif coroutine.status(afterFn) ~= "suspended"
						break
					elseif os.time() - start > Runner.TIMEOUT
						error("timeout after .. " .. tostring(Runner.TIMEOUT) .. "s\n")
					else
						Runner.debug("describe._verifyBehaviour: afterFn incomplete")
						coroutine.yield()


			Runner.debug("describe._verifyBehaviour: success=" .. tostring(success))
			if success
				io.write("... PASSED\n\n")
			else
				io.write("... FAILED\n" .. message .. "\n\n")

			@_runner.success = @_runner.success and success
		)


class Runner
	@TIMEOUT = 5
	@DEBUG: false
	debug: (msg)->
		if Runner.DEBUG then
			print(msg)

	new: ()=>
		@num_tests = 0
		@curr_test_num = 0
		@success = true
		@descriptions = {}
		@priority_mode = false
		@_asyncRunTests = @_createAsyncTestsRunner()

	describe: (subject, fn)=>
		return Describe(self, subject, fn)

	runTests: ()=>
		Runner.debug("runTests: starting tests")

		for _,description in ipairs(@descriptions)
			@num_tests = @num_tests + #description._behaviours

		io.write("Running " .. @num_tests .. " tests\n")

		return @resumeTests()

	resumeTests: ()=>
		if coroutine.status(@_asyncRunTests) == "dead" then return @success

		Runner.debug("resumeTests: resuming _asyncRunTests")
		success, message = coroutine.resume(@_asyncRunTests)

		Runner.debug(
			"resumeTests: Runner._asyncRunTests: success=" .. tostring(success) .. ", status=" ..
			coroutine.status(@_asyncRunTests)
		)

		if not success then
			Runner.debug("resumeTests: tests failed")
			io.write(message .. "\n")
			@success = false

		if coroutine.status(@_asyncRunTests) == "suspended" then return nil

		Runner.debug("resumeTests: tests succeeded")
		return @success

	_createAsyncTestsRunner: ()=>
		return coroutine.create(()->
			Runner.debug("_createAsyncTestsRunner: looping over describes")
			for _,description in ipairs(@descriptions)
				Runner.debug("_asyncRunTests: running: " .. description._subject)
				describeRunner = description\_runTests()
				while true
					Runner.debug("_asyncRunTests: describe resuming")
					success, message = coroutine.resume(describeRunner, description)
					if not success then
						Runner.debug("_asyncRunTests: describe failed")
						error("\n" .. message)
					elseif coroutine.status(describeRunner) == "suspended" then
						Runner.debug("_asyncRunTests: describe incomplete")
						coroutine.yield()
					else
						Runner.debug("_asyncRunTests: describe complete")
						break
		)
-- Runner.messageHandler = ( originalMessage )->
-- 	io.write(debug.traceback(originalMessage))


return Runner