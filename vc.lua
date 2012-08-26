function readmem (base, length)
	local current, retval
	for current = base, base + length do
		response, arr = vc("rm", current, length)
		if len(response) == 0 then break end
		length = length - string.len(response)
		retval = retval + response
	end
	return retval
end

function portscan()
	reponse, arr = vc("scan")
end

function loadedid(filename)
	local file = assert(io.open(filename, "r"))
	local t = f:read("*all")
	f:close()
	
	vc("wm", 0xf5a, t)
end
