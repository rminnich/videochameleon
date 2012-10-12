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

local clock = os.clock
function sleep(n)  -- seconds
  local t0 = clock()
  while clock() - t0 <= n do end
end

function dumppixels(x, y, w, h)
	maxpixels = HOST_BUFFER_SIZE / PIXEL_STORAGE_SIZE
	if (w * h > maxpixels) then
		error("Too many pixels requested!")
	else
		print("grabpixels")
		vc("grabpixels", 3, x, y, w, h)
		print("readmem")
		r, mem = readmem(HOST_BUFFER_LOCATION,
				PIXEL_STORAGE_SIZE * w * h)
		return parsepixels(mem, x, y, w, h)
	end
end

function parsepixels(mem, x, y, w, h)
	i = 1
	cx = x
	cy = y
	pixelnum = 1
	pixels = {}
	while (i < string.len(mem)) do
		color = {}
		for chan = 1,3 do
			color[chan] = string.byte(mem, i)
			color[chan] = color[chan] + (256 * string.byte(mem, i+1))
			i = i + 2
		end
		pixels[pixelnum] = color
		pixelnum = pixelnum + 1
		cx = cx + 1
		if (cx - x >= w) then
			cx = x
			cy = cy + 1
		end
	end
	return pixels
end

function saveimage(filename, x, y, w, h)
	maxpixels = HOST_BUFFER_SIZE / PIXEL_STORAGE_SIZE
	rowsperreq = math.min(math.floor(maxpixels / w), h)
	if (w > maxpixels) then
		error("Requested area is too wide!")
		return
	end

	of = io.open(filename,"w")
	of:write(string.format("P3\n%d %d\n1024\n", w, h))

	for req = 1,math.ceil(h / rowsperreq) do
		pixels = dumppixels(x, y + (req - 1) * rowsperreq, w, rowsperreq)
		p = 1

		for row = 1,rowsperreq do
			for col = 1,w do
				color = pixels[p]
				of:write(string.format("%d\t%d\t%d\t", color[1], color[2], color[3]))
				io.write(string.format("%d\t%d\t%d\n", color[1], color[2], color[3]))
				p = p + 1
			end
			of:write("\n")
		end
	end
	of:close()
end
