lassert = require 'luassert'
Runner = require 'feltest'
-- Runner.DEBUG = true

run = Runner()
calls = {}


assertCalls = (expected)->
	lassert.are.same(calls, expected)


run\describe('feltests', =>
	@beforeEach => table.insert(calls, "beforeEach inline #1")

	@afterEach => table.insert(calls, "afterEach inline #1")
	
	@beforeEach => table.insert(calls, "beforeEach inline #2")
			
	@afterEach => table.insert(calls, "afterEach inline #2")
	
	@it("does a test, inline", => 
		assertCalls({
			"beforeEach inline #1",
			"beforeEach inline #2"
			"beforeEach appended #1",
			"beforeEach appended #2"
		})
		table.insert(calls, "it inline #1")				
	)

	@it("does a second test, inline", => 
		assertCalls({
			"beforeEach inline #1",
			"beforeEach inline #2",
			"beforeEach appended #1",
			"beforeEach appended #2",
			"it inline #1",
			"afterEach inline #1",
			"afterEach inline #2",
			"afterEach appended #1",
			"afterEach appended #2",
			"beforeEach inline #1",
			"beforeEach inline #2"
			"beforeEach appended #1",
			"beforeEach appended #2"
		})
		table.insert(calls, "it inline #2")
	)

)\beforeEach(()=>
	table.insert(calls, "beforeEach appended #1")
	
)\afterEach(()=>
	table.insert(calls, "afterEach appended #1")
	
)\beforeEach(()=>
	table.insert(calls, "beforeEach appended #2")
	
)\afterEach(()=>
	table.insert(calls, "afterEach appended #2")
	
)\it("does a test, appended", ()=>
	assertCalls({
		"beforeEach inline #1",
		"beforeEach inline #2",
		"beforeEach appended #1",
		"beforeEach appended #2",
		"it inline #1",
		"afterEach inline #1",
		"afterEach inline #2",
		"afterEach appended #1",
		"afterEach appended #2",
		"beforeEach inline #1",
		"beforeEach inline #2"
		"beforeEach appended #1",
		"beforeEach appended #2"
		"it inline #2",
		"afterEach inline #1",
		"afterEach inline #2",
		"afterEach appended #1",
		"afterEach appended #2",
		"beforeEach inline #1",
		"beforeEach inline #2"
		"beforeEach appended #1",
		"beforeEach appended #2"
	})	
	table.insert(calls, "it appended #1")
)\it('does a second test, appended', ()=>	
	assertCalls({
		"beforeEach inline #1",
		"beforeEach inline #2",
		"beforeEach appended #1",
		"beforeEach appended #2",
		"it inline #1",
		"afterEach inline #1",
		"afterEach inline #2",
		"afterEach appended #1",
		"afterEach appended #2",
		"beforeEach inline #1",
		"beforeEach inline #2"
		"beforeEach appended #1",
		"beforeEach appended #2"
		"it inline #2",
		"afterEach inline #1",
		"afterEach inline #2",
		"afterEach appended #1",
		"afterEach appended #2",
		"beforeEach inline #1",
		"beforeEach inline #2"
		"beforeEach appended #1",
		"beforeEach appended #2"
		"it appended #1",
		"afterEach inline #1",
		"afterEach inline #2",
		"afterEach appended #1",
		"afterEach appended #2",
		"beforeEach inline #1",
		"beforeEach inline #2"
		"beforeEach appended #1",
		"beforeEach appended #2"
	})	
)


before_ran = 0
after_ran = 0
it_ran = 0

run\describe("async tests", =>
	@beforeEach( ()=>
		coroutine.yield()
		before_ran += 1
	)
	@afterEach( ()=>
		coroutine.yield()
		after_ran += 1
	)
	
	@it("runs a test async", ()=>
		coroutine.yield()
		it_ran += 1
	)
	
	@it("has run a test async", ()=>
		lassert.are.equal(before_ran, 2)
		lassert.are.equal(after_ran, 1)
		lassert.are.equal(it_ran, 1)	
	)
)

success = run\runTests()
-- Async so should be nil
lassert.is_nil(success)

resume_count = 0
while success == nil
	resume_count += 1

	success = run\resumeTests()

lassert.is.same(resume_count, 5)	

print("Tests completed with success=" .. tostring(success)) 
os.exit(sucess and 0 or 1)

