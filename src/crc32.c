#include "crc32.h"

// Based on an implementation found on hackersdelight.org, which was released
// into the public domain per the blanket license agreement for submitted code.

uint32_t crc32_bytes(const uint8_t *data, size_t len)
{
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < len; i++)
	{
		const uint32_t byte = data[i];
		crc = crc ^ byte;
		for (int j = 7; j >= 0; j--)
		{
			const uint32_t mask = ~(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}
	return ~crc;
}
