EDID_MEMORY_LOCATION = 0x0f5a

MAX_WRITE_LEN = 200

function readmem (base, length)
	local current, retval
	for current = base, base + length do
		response, arr = vc("rm", current, length)
		if string.len(response) == 0 then break end
		length = length - string.len(response)
		retval = retval + response
	end
	return retval
end

function writemem (base, s)
	local addr = base
	while string.len(s) > 0 do
		vc("wm", addr, string.sub(s, 1, MAX_WRITE_LEN))
		s = string.sub(s, MAX_WRITE_LEN)
		addr = addr + MAX_WRITE_LEN
	end
end

function portscan()
	reponse, arr = vc("scan")
end

function loadedid(filename)
	local f = assert(io.open(filename, "r"))
	local t = f:read("*all")
	f:close()
	
	writemem(EDID_MEMORY_LOCATION, t)
end
