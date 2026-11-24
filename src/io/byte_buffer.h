#ifndef IO_BYTE_BUFFER_H
#define IO_BYTE_BUFFER_H

#include "core/types.h"
#include "container/vector.h"
#include "container/string.h"

class ByteWriter {
public:
	ByteWriter() = default;

	void WriteBytes(const void *data, u64 size);

	void WriteU8(u8 value);
	void WriteBool(bool value);
	void WriteU16(u16 value);
	void WriteI32(i32 value);
	void WriteU32(u32 value);
	void WriteI64(i64 value);
	void WriteU64(u64 value);
	void WriteFloat(float value);
	void WriteDouble(double value);

	// optimize positive is zigzag encoded such that small negatives are short too
	void WriteVarInt(i32 value, bool optimize_positive);

	void WriteString(const String &value);

	// u32 length followed by the raw bytes
	void WriteBlob(const Vector<u8> &value);

	[[nodiscard]] const Vector<u8> &Data() const
	{
		return data;
	}

	Vector<u8> &Data()
	{
		return data;
	}

	[[nodiscard]] u64 Size() const
	{
		return data.size();
	}

	void Clear()
	{
		data.clear();
	}

private:
	Vector<u8> data;
};

class ByteReader {
public:
	ByteReader(const void *data, u64 size);
	explicit ByteReader(const Vector<u8> &data);

	[[nodiscard]] bool Ok() const
	{
		return !failed;
	}

	[[nodiscard]] u64 Remaining() const
	{
		return size - offset;
	}

	[[nodiscard]] u64 Tell() const
	{
		return offset;
	}

	bool ReadBytes(void *out, u64 count);

	u8 ReadU8();
	bool ReadBool();
	u16 ReadU16();
	i32 ReadI32();
	u32 ReadU32();
	i64 ReadI64();
	u64 ReadU64();
	float ReadFloat();
	double ReadDouble();

	i32 ReadVarInt(bool optimize_positive);

	String ReadString();
	Vector<u8> ReadBlob();

private:
	const u8 *data;
	u64 size;
	u64 offset;
	bool failed;
};

#endif // IO_BYTE_BUFFER_H
