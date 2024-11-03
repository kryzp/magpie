#include "endian.h"
#include "../common.h"

using namespace llt;

Endianness endian::getEndianness()
{
	return isBigEndian()
		? ENDIANNESS_BIG
		: ENDIANNESS_LITTLE;
}

bool endian::isEndian(Endianness endian)
{
	return (
		(endian == ENDIANNESS_LITTLE && isLittleEndian()) ||
		(endian == ENDIANNESS_BIG && isBigEndian())
	);
}

// compare two c-strings. depending on the way they are represented in memory tells us whether we are on a little or big endian system.
bool endian::isLittleEndian()	{ return *(reinterpret_cast<const uint16_t*>("AB")) == 0x4241; }
bool endian::isBigEndian()	{ return *(reinterpret_cast<const uint16_t*>("AB")) == 0x4142; }
