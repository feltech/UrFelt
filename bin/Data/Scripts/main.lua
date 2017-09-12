local root = GetFileSystem().GetProgramDir()
package.path = root .. 'Data/Scripts/?.lua;' .. root .. 'Data/Scripts/lib/?.lua;' .. root .. 'Data/Scripts/lib/?/init.lua;'
return require('test')
