#ifndef ENDIAN_H_
#define ENDIAN_H_

namespace llt
{
	enum Endianness
	{
		ENDIANNESS_LITTLE,
		ENDIANNESS_BIG,
	};

	/**
	 * Utility for querying the endianness of the system.
	 */
	namespace endian
	{
		/*
		 * Finds the endianness of our system.
		 */
		Endianness getEndianness();

		bool isEndian(Endianness endian);
		bool isLittleEndian();
		bool isBigEndian();
	};
}

#endif // ENDIAN_H_
