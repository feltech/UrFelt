local format_call_status = require 'mach.format_call_status'
local format_arguments = require 'mach.format_arguments'

return function(name, args, completed_calls, incomplete_calls, level)
  local message =
    'Unexpected function call ' .. name .. format_arguments(args) ..
    format_call_status(completed_calls, incomplete_calls)

  error(message, level + 1)
end
