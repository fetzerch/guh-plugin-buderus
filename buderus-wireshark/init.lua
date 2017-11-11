-- Import dependencies.
local gcrypt
do
    local status, res = pcall(require, "luagcrypt")
    if status then
        gcrypt = res
    else
        report_failure("Failed to load Luagcrypt. See README.md.\n" .. res)
    end
end
local json = require "json"

local function buderus_dissect(proto, tvbuffer, treeitem)
    local subtree = treeitem:add(proto, tvbuffer(), "Buderus KM200 Packet")
    local payload = tvbuffer():string()

    -- Remove preceding HTTP header and following XMPP/XML tags.
    local hdr_len = payload:find("\r\n\r\n")
    if hdr_len == nil then
        return
    end
    local content = payload:sub(hdr_len + 4)
    local index = content:find('<')
    if index ~= nil then
        content = content:sub(0, content:find('<') - 1)
    end
    subtree:add("Data: " .. content)

    -- Decrypt content.
    local cipher = gcrypt.Cipher(gcrypt.CIPHER_AES256, gcrypt.CIPHER_MODE_ECB)
    if not pcall(function()
        cipher:setkey(Struct.fromhex(proto.prefs.key))
    end) then
        subtree:add("Invalid decryption key set in protocol preferences.")
        return
    end
    local decoded = ByteArray.new(content, true):base64_decode()
    local decrypted = cipher:decrypt(decoded:raw())
    subtree:add("Decrypted Data: " .. decrypted)

    -- Parse the decrypted packet as JSON.
    local json_obj
    if not pcall(function()
        json_obj = json.decode(string.match(decrypted, "{.*}"))
    end) then
        return
    end
    local subtree2 = subtree:add("JSON Contents:")
    local function json_add(object, tree)
        for key, value in pairs(object) do
            if not(type(value) == "table") then
                tree:add(key .. ": " .. value)
            else
                subtree = tree:add(key .. ":")
                json_add(value, subtree)
            end
        end
    end
    json_add(json_obj, subtree2)
end

local buderus_http_proto = Proto("buderus_http","Buderus KM200 (HTTP)")
buderus_http_proto.prefs.key = Pref.string( "Key", "", "AES256 Decryption Key")
local original_http_dissector
function buderus_http_proto.dissector(tvbuffer, pinfo, treeitem)
    -- Call stored http dissector so that its output is available as well.
    -- https://osqa-ask.wireshark.org/questions/57685/reassembly-not-working-with-a-chained-dissector-in-lua
    local can_desegment_saved = pinfo.can_desegment
    if pinfo.can_desegment > 0 then
        pinfo.can_desegment = 2
    end
    original_http_dissector:call(tvbuffer, pinfo, treeitem)
    pinfo.can_desegment = can_desegment_saved

    buderus_dissect(buderus_http_proto, tvbuffer, treeitem)
end

local buderus_xmpp_proto = Proto("buderus_xmpp","Buderus KM200 (XMPP)")
buderus_xmpp_proto.prefs.key = Pref.string( "Key", "", "AES256 Decryption Key")
local original_xmpp_dissector
function buderus_xmpp_proto.dissector(tvbuffer, pinfo, treeitem)
    -- Call stored xmpp dissector so that its output is available as well.
    -- https://osqa-ask.wireshark.org/questions/57685/reassembly-not-working-with-a-chained-dissector-in-lua
    local can_desegment_saved = pinfo.can_desegment
    if pinfo.can_desegment > 0 then
        pinfo.can_desegment = 2
    end
    original_xmpp_dissector:call(tvbuffer, pinfo, treeitem)
    pinfo.can_desegment = can_desegment_saved

    buderus_dissect(buderus_xmpp_proto, tvbuffer, treeitem)
end

local tcp_dissector_table = DissectorTable.get("tcp.port")
original_http_dissector = Dissector.get("http")
original_xmpp_dissector = Dissector.get("xmpp")
tcp_dissector_table:add(80, buderus_http_proto)
tcp_dissector_table:add(5222, buderus_xmpp_proto)
