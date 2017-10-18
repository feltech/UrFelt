export print = (str)->
	Log\Write(LOG_INFO, str)

root = GetFileSystem().GetProgramDir()
package.path = root .. 'Data/Scripts/?.lua;' ..root .. 'Data/Scripts/lib/?.lua;' ..
	root .. 'Data/Scripts/lib/?/init.lua;'

require 'brain'
-- require 'segment'
-- require 'test'
-- require 'feltest.spec'
