export print = (str)->
	Log\Write(LOG_INFO, str)

root = GetFileSystem().GetProgramDir()
package.path = root .. 'Data/Scripts/?.lua;' ..root .. 'Data/Scripts/lib/?.lua;' ..
	root .. 'Data/Scripts/lib/?/init.lua;'

-- Demos, making use of MRI image from BrainWeb.
require 'brain'
-- require 'segment'

-- Unit tests
-- require 'test'
-- require 'feltest.spec'
