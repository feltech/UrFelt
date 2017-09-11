root = GetFileSystem().GetProgramDir()
-- print package.path
package.path = root .. 'Data/Scripts/?.lua'

require 'test'