root = GetFileSystem().GetProgramDir()
-- package.path = root .. 'vendor/share/lua/5.1/?.lua;'.. 
-- 	root .. 'vendor/share/lua/5.1/?/init.lua;' .. 
-- 	root .. 'vendor/lib/lua/5.1/?.so;'.. 
-- 	root .. 'vendor/lib/lua/5.1/?/init.so;' .. 
-- 	package.path

-- I'm a comment!
print "Hello from MoonScript!"
 
class UrFelt
	classvar: 2
	@instancevar: 3
	printvars: (paramvar)=>
		print(classvar)
		print(@instancevar)
		print(paramvar)
