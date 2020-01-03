-- root dir
__ROOT__ = __api__.root
-- set require path
package.path = __api__.pdir .. '/lua/?.lua;'..__api__.apiroot..'/?.lua'
package.cpath = __api__.pdir..'/lua/?.so'

require("antd")
require("utils")
std = modules.std()
bytes = modules.bytes()
array = modules.array()

std.ws = {}

std.ws.TEXT = 1
std.ws.BIN = 2
std.ws.CLOSE = 8

if __api__.pending then
    -- we have data
    local h = std.ws_header(__api__.id)
    if h and h.opcode == std.ws.BIN then
        local data = std.ws_read(__api__.id, h)
        for i = 1,bytes.size(data) do
            print(data[i])
        end
        std.ws_b(__api__.id, data)
    else
        std.ws_close(__api__.id, 1011)
        return false
    end
end

return true