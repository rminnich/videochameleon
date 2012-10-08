MAX_MEM_LEN = 200
HOST_BUFFER_LOCATION = 0x0f5a
HOST_BUFFER_SIZE = 0x400
PIXEL_STORAGE_SIZE = 6
EDID_MEMORY_LOCATION = HOST_BUFFER_LOCATION
EDID_LENGTH = 128

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

function dumphex(val)
	i = 1
	offset = 0
	while (i <= string.len(val)) do
		j = 1
		io.write(string.format("%04X: ", offset))
		while (i <= string.len(val) and j <= 16) do
			io.write(string.format("%02X ", string.byte(val, i)))
			i = i + 1
			j = j + 1
		end
		io.write("\n")
		offset = offset + 16
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

	vc("edidmanipulation", 2)
end

function getcurrentedid()
	r, edid = readmem(EDID_MEMORY_LOCATION, EDID_LENGTH)
	return edid
end

function dumppixels(x, y, w, h)
	maxpixels = HOST_BUFFER_SIZE / PIXEL_STORAGE_SIZE
	if (w * h > maxpixels) then
		error("Too many pixels requested!")
	else
		vc("grabpixels", 3, x, y, w, h)
		r, mem = readmem(HOST_BUFFER_LOCATION,
				PIXEL_STORAGE_SIZE * w * h)
		return mem
	end
end

function parsepixels(mem, x, y, w, h)
	i = 1
	cx = x
	cy = y
	print("Pixel\tRed\tGreen\tBlue")
	while (i < string.len(mem)) do
		io.write(string.format("(%d, %d)\t", cx, cy))
		for channel = 0,2 do
			value = string.byte(mem, i)
			value = value + (256 * string.byte(mem, i+1))
			io.write(string.format("%d\t", value))
			i = i + 2
		end
		io.write(string.format("\n"))
		cx = cx + 1
		if (cx - x >= w) then
			cx = x
			cy = cy + 1
		end
	end
end

function readhostbuffer()
	r, mem = readmem(HOST_BUFFER_LOCATION, HOST_BUFFER_SIZE)
	return mem
end
