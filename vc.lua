function readmem (base, length)
	local current, retval
	for current = base, base + length do
		response, arr = cv("rm", current, length)
		if len(response) == 0 then break end
		length = length - string.len(response)
		retval = retval + response
	end
	return retval
end