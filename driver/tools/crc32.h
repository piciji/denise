
#pragma once

struct CRC32 {

	CRC32(uint8_t* data, unsigned size) {
		auto p = data;
		
		while (size--) {
			checksum = (checksum >> 8) ^ table(checksum ^ *p++);
		}
	}

	auto value() const -> uint32_t {
		return ~checksum;
	}
	
private:
	static auto table(uint8_t index) -> uint32_t {
		
		static uint32_t table[256] = {0};
		static bool initialized = false;

		if (!initialized) {
			initialized = true;
			for (auto i = 0; i < 256; i++) {
				uint32_t crc = i;
				for (auto bit = 0; bit < 8; bit++) {
					crc = (crc >> 1) ^ (crc & 1 ? 0xedb88320 : 0);
				}
				table[i] = crc;
			}
		}

		return table[index];
	}

	uint32_t checksum = 0;
};