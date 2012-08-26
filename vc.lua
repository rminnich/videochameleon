EDID_MEMORY_LOCATION = 0x0f5a

MAX_MEM_LEN = 200

function readmem (base, length)
	local current, retval
	current = base
	retval = ""
	endaddr = base + length
	while current < endaddr do
		amt = length
		if amt > MAX_MEM_LEN then amt = MAX_MEM_LEN end
		response, arr = vc("getmem", current, amt)
		amt = string.len(arr)
		if amt == 0 then break end
		length = length - amt
		current = current + amt
		retval = retval .. arr
	end
	return response,retval
end

function writemem (base, s)
	local addr = base
	while string.len(s) > 0 do
		vc("wm", addr, string.sub(s, 1, MAX_MEM_LEN))
		s = string.sub(s, MAX_MEM_LEN)
		addr = addr + MAX_MEM_LEN
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
