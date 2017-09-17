root = GetFileSystem().GetProgramDir()
-- print package.path
package.path = root .. 'Data/Scripts/?.lua;' ..root .. 'Data/Scripts/lib/?.lua;' ..
	root .. 'Data/Scripts/lib/?/init.lua;'

-- require 'feltest.spec'
require 'test'