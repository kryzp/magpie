#include "io/byte_buffer.h"

#include <string.h>

void ByteWriter::WriteBytes(const void *src, u64 count)
{
	const u8 *bytes = static_cast<const u8 *>(src);
	data.insert(data.end(), bytes, bytes + count);
}

void ByteWriter::WriteU8(u8 value)
{
	data.push_back(value);
}

void ByteWriter::WriteBool(bool value)
{
	data.push_back(value ? 1 : 0);
}

void ByteWriter::WriteU16(u16 value)
{
	data.push_back(static_cast<u8>(value >> 8));
	data.push_back(static_cast<u8>(value));
}

void ByteWriter::WriteI32(i32 value)
{
	WriteU32(static_cast<u32>(value));
}

void ByteWriter::WriteU32(u32 value)
{
	data.push_back(static_cast<u8>(value >> 24));
	data.push_back(static_cast<u8>(value >> 16));
	data.push_back(static_cast<u8>(value >> 8));
	data.push_back(static_cast<u8>(value >> 0));
}

void ByteWriter::WriteI64(i64 value)
{
	WriteU64(static_cast<u64>(value));
}

void ByteWriter::WriteU64(u64 value)
{
	WriteU32(static_cast<u32>(value >> 32));
	WriteU32(static_cast<u32>(value));
}

void ByteWriter::WriteFloat(float value)
{
	u32 bits;
	memcpy(&bits, &value, sizeof(bits));
	WriteU32(bits);
}

void ByteWriter::WriteDouble(double value)
{
	u64 bits;
	memcpy(&bits, &value, sizeof(bits));
	WriteU64(bits);
}

void ByteWriter::WriteVarInt(i32 value, bool optimize_positive)
{
	u32 unsigned_value = static_cast<u32>(value);

	if (!optimize_positive) {
		// zigzag
		u32 sign_mask = static_cast<u32>(value >> 31);
		unsigned_value = (unsigned_value << 1) ^ sign_mask;
	}

	while (unsigned_value >= 0x80) {
		data.push_back(static_cast<u8>((unsigned_value & 0x7F) | 0x80));
		unsigned_value >>= 7;
	}

	data.push_back(static_cast<u8>(unsigned_value));
}

void ByteWriter::WriteString(const String &value)
{
	u64 clamped = value.size();

	if (clamped > 0xFFFF)
		clamped = 0xFFFF;

	u16 length = static_cast<u16>(clamped);
	WriteU16(length);
	WriteBytes(value.data(), length);
}

void ByteWriter::WriteBlob(const Vector<u8> &value)
{
	WriteU32(static_cast<u32>(value.size()));
	WriteBytes(value.data(), value.size());
}

ByteReader::ByteReader(const void *data_, u64 size_)
	: data(static_cast<const u8 *>(data_))
	, size(size_)
	, offset(0)
	, failed(false)
{
}

ByteReader::ByteReader(const Vector<u8> &data_)
	: data(data_.data())
	, size(data_.size())
	, offset(0)
	, failed(false)
{
}

bool ByteReader::ReadBytes(void *out, u64 count)
{
	if (failed || count > size - offset) {
		failed = true;
		memset(out, 0, count);
		return false;
	}

	memcpy(out, data + offset, count);
	offset += count;

	return true;
}

u8 ByteReader::ReadU8()
{
	u8 v = 0;
	ReadBytes(&v, 1);
	return v;
}

bool ByteReader::ReadBool()
{
	return ReadU8() != 0;
}

u16 ByteReader::ReadU16()
{
	u8 b[2];
	ReadBytes(b, 2);
	return static_cast<u16>((b[0] << 8) | b[1]);
}

i32 ByteReader::ReadI32()
{
	return static_cast<i32>(ReadU32());
}

u32 ByteReader::ReadU32()
{
	u8 b[4];
	ReadBytes(b, 4);

	return
		(static_cast<u32>(b[0]) << 24) |
		(static_cast<u32>(b[1]) << 16) |
		(static_cast<u32>(b[2]) <<  8) |
		(static_cast<u32>(b[3]) <<  0);
}

i64 ByteReader::ReadI64()
{
	return static_cast<i64>(ReadU64());
}

u64 ByteReader::ReadU64()
{
	u64 hi = ReadU32();
	u64 lo = ReadU32();
	return (hi << 32) | lo;
}

float ByteReader::ReadFloat()
{
	u32 bits = ReadU32();
	float value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

double ByteReader::ReadDouble()
{
	u64 bits = ReadU64();
	double value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

i32 ByteReader::ReadVarInt(bool optimize_positive)
{
	u32 result = 0;

	for (u32 shift = 0; shift < 35; shift += 7) {
		u8 b = ReadU8();

		if (failed)
			return 0;

		result |= static_cast<u32>(b & 0x7F) << shift;

		if ((b & 0x80) == 0)
			break;
	}

	if (optimize_positive)
		return static_cast<i32>(result);

	return static_cast<i32>((result >> 1) ^ static_cast<u32>(-static_cast<i32>(result & 1)));
}

String ByteReader::ReadString()
{
	u16 length = ReadU16();

	if (failed || length > size - offset) {
		failed = true;
		return String();
	}

	String result(reinterpret_cast<const char *>(data) + offset, length);
	offset += length;

	return result;
}

Vector<u8> ByteReader::ReadBlob()
{
	u32 length = ReadU32();

	if (failed || length > size - offset) {
		failed = true;
		return Vector<u8>();
	}

	Vector<u8> result(data + offset, data + offset + length);
	offset += length;

	return result;
}
