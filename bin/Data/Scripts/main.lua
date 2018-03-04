print = function(str)
  return Log:Write(LOG_INFO, str)
end
local root = GetFileSystem().GetProgramDir()
package.path = root .. 'Data/Scripts/?.lua;' .. root .. 'Data/Scripts/lib/?.lua;' .. root .. 'Data/Scripts/lib/?/init.lua;'
return require('brain')
