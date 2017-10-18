lassert = require 'luassert'
stub = require 'luassert.stub'
DebugScene = require 'debug_scene'
Runner = require 'feltest'

Runner.DEBUG = false
Runner.TIMEOUT = 120

run = Runner()
debug_scene = DebugScene()


await_finish = nil
snapshot = nil


run\describe(
	'stubbing userdata'

)\beforeEach( () ->
	snapshot = lassert\snapshot()

)\afterEach( () ->
	snapshot\revert()

)\it('successfully stubs userdata', () ->

	s = stub(getmetatable(input), "SetMouseVisible")

	input\SetMouseVisible(true)

	lassert.is_not_nil(getmetatable(input.SetMouseVisible))
	lassert.stub(s).was.called_with(input, true)

)\it('removes the stub when done', () ->

	lassert.is_nil(getmetatable(input.SetMouseVisible))
)


run\describe "surface", =>

	@beforeEach => debug_scene\recreate()

	@it "can be constructed", =>
		node = debug_scene.scene\CreateChild("Surface")
		@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)

		lassert.are.equal(type(@surface), "userdata", "surface should be userdata")

	@it "can spawn a seed", =>
		node = debug_scene.scene\CreateChild("Surface")
		@surface = UrFelt.UrSurface(IntVector3(16, 16, 16), IntVector3(8, 8, 8), node)

		@surface\seed(IntVector3(0,0,0))

	@describe "ray", =>
		@beforeEach =>
			node = debug_scene.scene\CreateChild("Surface")
			@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
			@surface\seed(IntVector3(0,0,0))
			@surface\expand_by_constant(-1)
			@surface\await()

		@it "can cast to surface", =>
			ray = debug_scene.camera\GetScreenRay(0.5, 0.5)

			pos_hit = @surface\ray(ray)

			lassert.is_equal(
				pos_hit, Vector3(0,0,-1),
				Vector3(pos_hit)\ToString() .. " != " .. Vector3(0,0,0)\ToString()
			)

	@describe "async", =>
		@beforeEach () =>
			node = debug_scene.scene\CreateChild("Surface")
			@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
			@surface\seed(IntVector3(0,0,0))

		@it "blocks on await", =>
			finished = false
			transformed = false

			-- No callback.
			@surface\expand_by_constant(-1)

			-- With a callback.
			@surface\expand_by_constant(-1, -> transformed = true)

			-- Finish with a polygonise and flush in main thread.
			@surface\polygonise ->
				@surface\flush()
				finished = true

			lassert.is_false(transformed)
			lassert.is_false(finished)

			coroutine.yield()

			lassert.is_false(finished)
			lassert.is_false(transformed)

			@surface\await()

			lassert.is_true(finished)
			lassert.is_true(transformed)


		@it "doesn't block on poll", =>
			finished = false

			-- With a callback.
			@surface\expand_by_constant -5, ->
				@surface\polygonise ->
					@surface\flush()
					finished = true

			lassert.is_false(finished)

			@surface\poll()
			coroutine.yield()

			lassert.is_false(finished)

			count = 0

			while not finished
				count = count + 1
				@surface\poll()
				coroutine.yield()

			lassert.is_true(count > 1)

			print("Iterations " .. tostring(count))


	@describe "ops", =>
		@beforeEach () =>
			@finished = false
			@await_finish = await_finish

			node = debug_scene.scene\CreateChild("Surface")

			@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), node)
			@surface\seed(IntVector3(0,0,0))
			@surface\expand_by_constant(-1)


		@describe "transform by constant", =>
			@it "can transform a small region", =>

				@surface\expand_by_constant Vector3(-1, -1, 1), Vector3(1, 1, 1), -1.0, ->
					@surface\polygonise ->
						@surface\flush()
						@finished = true

				@await_finish()

				pos_hit = @surface\ray(debug_scene.camera\GetScreenRay(0.5, 0.5))
				lassert.is_equal(pos_hit, Vector3(0,0,-1))

		@describe "transform to box", =>
			@it "can fill a box", =>

				@surface\transform_to_box Vector3(-10,-5,-10), Vector3(10,5,10), ->
					@surface\polygonise ->
						@surface\flush()
						@finished = true

				@await_finish()

			@it "can move a box to fill a different box", =>
				@surface\transform_to_box Vector3(3,3,3), Vector3(11,11,11), ->
					@surface\transform_to_box Vector3(-10,-10,-10), Vector3(0,3,0), ->
						@surface\polygonise ->
							@surface\flush()
							@finished = true

				@await_finish()

		@describe "attract to sphere", =>
			@it "can expand towards a sphere", =>

				@surface\transform_to_box Vector3(-5,-5,-5), Vector3(5,5,5), ->
					@surface\attract_to_sphere Vector3(1,1,-5), 3, ->
						@surface\polygonise ->
							@surface\flush()
							@finished = true


				@await_finish()

		@describe "repel from sphere", =>
			@it "can contract towards a sphere", =>

				@surface\transform_to_box Vector3(-5,-5,-5), Vector3(5,5,5), ->
					@surface\repel_from_sphere Vector3(1,1,-5), 3, ->
						@surface\polygonise ->
							@surface\flush()
							@finished = true

				@await_finish()

	@describe "serialisation", =>
		@beforeEach () =>
			@node = debug_scene.scene\CreateChild("Surface")
			@finished = false
			@await_finish = await_finish

		@it "can save to disk", =>
			@surface = UrFelt.UrSurface(IntVector3(32, 32, 32), IntVector3(8, 8, 8), @node)
			@surface\seed(IntVector3(0,0,0))
			@surface\expand_by_constant(-1)

			@surface\transform_to_box Vector3(-2,-2,-2), Vector3(2,2,2), ->
				@surface\save "/tmp/urfelt.test.bin", ->
					@finished = true

			@await_finish()

		@it "can load from disk", =>
			loader = UrFelt.UrSurface.load("/tmp/urfelt.test.bin", @node)

			while not loader\ready()
				io.write(".")
				coroutine.yield()

			io.write("\n")
			@surface = loader\get()

			@surface\invalidate()
			@surface\polygonise()
			@surface\await()
			@surface\flush()


	@describe "image segmentation", =>
		@it 'transforms to fit image until cancelled', ()=>
			finished = false
			start_time = now()

			node = debug_scene.scene\CreateChild("Surface")
			@surface = UrFelt.UrSurface(
				IntVector3(200, 200, 200), IntVector3(20, 20, 20), node
			)
			@surface\seed(IntVector3(50,50,50))
			@surface\seed(IntVector3(-50,-50,-50))
			@surface\seed(IntVector3(50,50,-50))
			@surface\expand_by_constant(-1)

			op = @surface\transform_to_image "brain.hdr", 0.58, 0.2, 0.2, -> finished = true

			last = now()
			is_rendering = false
			count = 0
			while not finished
				count = count + 1
				if now() - last > 100
					if not is_rendering
						is_rendering = true
						@surface\polygonise ->
							last = now()
							@surface\flush_graphics()
							is_rendering = false
					@surface\poll()

				if now() - start_time > 1000
					op\stop()

				coroutine.yield()

			print("Segmentation took " .. tostring(now() - start_time) .. " ms")


await_finish = ()=>
	last = now()
	count = 0
	is_rendering = false
	while not @finished
		count = count + 1
		if now() - last > 100
			if not is_rendering
				is_rendering = true
				@surface\polygonise ->
					last = now()
					@surface\flush()
					is_rendering = false

			@surface\poll()

		coroutine.yield()

	has_final_render = false
	@surface\polygonise ->
		@surface\flush()
		has_final_render = true

	while not has_final_render
		@surface\poll()
		coroutine.yield()

	print("Iterations " .. tostring(count))


resumeTesting = ()->

	success = run\resumeTests()
	if success ~= nil
		print("Tests completed with " .. if success then "success" else "failure")
		os.exit(sucesss and 0 or 1)


debug_scene\subscribe_to_update(resumeTesting)


run\runTests()







