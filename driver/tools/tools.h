
#pragma once

template<unsigned bits> inline auto sclamp(const signed x) -> signed {
    enum : signed { b = 1U << (bits - 1), m = b - 1 };
    return (x > m) ? m : (x < -b) ? -b : x;
}

template<typename T> inline auto MatrixMultiply(T* output, const T* xdata, unsigned xrows, unsigned xcols, const T* ydata, unsigned yrows, unsigned ycols) -> void {
	if(xcols != yrows) return;

	for(unsigned y = 0; y < xrows; y++) {
		for(unsigned x = 0; x < ycols; x++) {
			T sum = 0;
			for(unsigned z = 0; z < xcols; z++) {
				sum += xdata[y * xcols + z] * ydata[z * ycols + x];
			}
			*output++ = sum;
		}
	}
}

template<typename T> auto uniqueDeviceId(std::vector<T>& devices, uint64_t id) -> uint64_t {
	for (auto& device : devices) {
		if (device.hid->id == id)
			return uniqueDeviceId(devices, id + 1);
	}
	return id;
}

template<typename T> auto uniqueDeviceName(std::vector<T>& devices, std::string name, unsigned i = 1) -> std::string {
	std::string cmpname = i == 1 ? name : name + "." + std::to_string(i);
	
	for (auto& device : devices) {
		if (device.hid->name == cmpname)
			return uniqueDeviceName(devices, name, i+1);
	}
	return cmpname;
}

constexpr inline auto roundUpPowerOfTwo(uintmax_t x) -> uintmax {
	
	if( (x & (x - 1) ) == 0 )
		return x;
	
	while(x & (x - 1))
		x &= x - 1;
	
	return x << 1;
}
