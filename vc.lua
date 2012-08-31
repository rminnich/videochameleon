EDID_MEMORY_LOCATION = 0x0f5a
EDID_LENGTH = 128
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
	local bytes_written = 0
	while string.len(s) > 0 do
		vc("wm", addr, string.sub(s, 1, MAX_MEM_LEN))
		bytes_written = bytes_written + math.min(MAX_MEM_LEN, string.len(s))
		s = string.sub(s, MAX_MEM_LEN)
		addr = addr + MAX_MEM_LEN
	end
	return bytes_written
end

function portscan()
	reponse, arr = vc("scan")
end

function loadedid(filename)
	local f = assert(io.open(filename, "r"))
	local t = f:read("*all")
	local bytes_written
	f:close()
	
	return writemem(EDID_MEMORY_LOCATION, t)
end

function getcurrentedid()
	r, edid = readmem(EDID_MEMORY_LOCATION, EDID_LENGTH)
	return edid
end
