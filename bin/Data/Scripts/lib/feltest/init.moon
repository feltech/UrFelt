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

			if @_runner.priority_mode
				@_behaviours = @_priority_behaviours

			for behaviour in *@_behaviours
				verify = @_verifyBehaviour(behaviour)

				while true
					success, message = coroutine.resume(verify)
					if not success then
						error("\n" .. message)
					elseif coroutine.status(verify) == "suspended" then
						coroutine.yield()
					else
						break
		)

	_verifyBehaviour: (behaviour)=>
		return coroutine.create(()->
			start = os.time()
			@_runner.curr_test_num += 1
			io.write("[" .. @_runner.curr_test_num .. "] " .. behaviour.description .. " ...\n")

			beforeFns = [coroutine.create(fn) for fn in *@_beforeFns]
			afterFns = [coroutine.create(fn) for fn in *@_afterFns]
			for beforeFn in *beforeFns
				start = os.time()
				while true
					is_ok, err = coroutine.resume(beforeFn, behaviour)
					if not is_ok
						error("\n" .. err)
					elseif coroutine.status(beforeFn) ~= "suspended"
						break
					elseif os.time() - start > Runner.TIMEOUT
						error("timeout after " .. tostring(Runner.TIMEOUT) .. "s")
					else
						coroutine.yield()


			start = os.time()
			success = false
			message = nil
			while true
				success, message = coroutine.resume(behaviour.testFn, behaviour)
				if not success
					break
				elseif coroutine.status(behaviour.testFn) ~= "suspended"
					break
				elseif os.time() - start > Runner.TIMEOUT
					success = false
					message = "timeout after " .. tostring(Runner.TIMEOUT) .. "s"
					break
				else
					coroutine.yield()


			for afterFn in *afterFns
				start = os.time()
				while true
					is_ok, err = coroutine.resume(afterFn, behaviour)
					if not is_ok
						error("\n" .. err)
					elseif coroutine.status(afterFn) ~= "suspended"
						break
					elseif os.time() - start > Runner.TIMEOUT
						error("timeout after .. " .. tostring(Runner.TIMEOUT) .. "s\n")
					else
						coroutine.yield()


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

		for _,description in ipairs(@descriptions)
			@num_tests = @num_tests + #description._behaviours

		io.write("Running " .. @num_tests .. " tests\n")

		return @resumeTests()

	resumeTests: ()=>
		if coroutine.status(@_asyncRunTests) == "dead" then return @success

		success, message = coroutine.resume(@_asyncRunTests)

		if not success then
			io.write(message .. "\n")
			@success = false

		if coroutine.status(@_asyncRunTests) == "suspended" then return nil

		return @success

	_createAsyncTestsRunner: ()=>
		return coroutine.create(()->
			for _,description in ipairs(@descriptions)
				describeRunner = description\_runTests()
				while true
					success, message = coroutine.resume(describeRunner, description)
					if not success then
						error("\n" .. message)
					elseif coroutine.status(describeRunner) == "suspended" then
						coroutine.yield()
					else
						break
		)
-- Runner.messageHandler = ( originalMessage )->
-- 	io.write(debug.traceback(originalMessage))


return Runner