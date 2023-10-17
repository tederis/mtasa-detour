#pragma once

#include <vector>
#include <ostream>
#include <fstream>

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

static_assert(sizeof(float) == 4, "Invalid sizeof(float)");

class InputStream
{
public:
	explicit InputStream();

	virtual std::size_t Read(void* data, std::size_t len);

	std::string ReadFileID();

	std::uint8_t ReadUByte();

	std::int32_t ReadInt();

	std::uint32_t ReadUInt();

	std::uint64_t ReadUInt64();

	float ReadFloat();

	Vector3F ReadVector3();

	BoundingBox ReadBoundingBox();

	std::int16_t ReadShort();

	std::uint16_t ReadUShort();

	std::string ReadString();

	virtual std::size_t Seek(std::size_t pos, bool relative = false);

	virtual std::size_t Tell() const;

	virtual bool Eof() const;

protected:
	std::size_t size_{};
};

class InputFileStream : public InputStream
{
public:
	InputFileStream(std::ifstream& stream);

	std::size_t Read(void* data, std::size_t len) override;

	std::size_t Seek(std::size_t pos, bool relative = false) override;

	std::size_t Tell() const override;

	bool Eof() const override;

private:
	std::ifstream& stream_;
};

class InputMemoryStream : public InputStream
{
public:
	InputMemoryStream(const std::vector<std::uint8_t>& buffer);

	InputMemoryStream(const std::uint8_t* buffer, std::size_t size);

	std::size_t Read(void* data, std::size_t len) override;

	std::size_t Seek(std::size_t pos, bool relative = false) override;

	std::size_t Tell() const override;

	bool Eof() const override;

private:
	const std::uint8_t* buffer_{};

	size_t pos_{};
};

class OutputStream
{
public:
	explicit OutputStream();

	virtual std::size_t Write(const void* data, std::size_t len);

	std::size_t WriteFileID(const char* value);

	std::size_t WriteBool(bool value);

	std::size_t WriteUByte(std::uint8_t value);

	std::size_t WriteUInt(std::uint32_t value);

	std::size_t WriteUInt64(std::uint64_t value);

	std::size_t WriteInt(std::int32_t value);

	std::size_t WriteUShort(std::uint16_t value);

	std::size_t WriteShort(std::int16_t value);

	std::size_t WriteFloat(float value);

	std::size_t WriteVector3(const Vector3F& value);

	std::size_t WriteBoundingBox(const BoundingBox& bounds);

	std::size_t WriteString(const std::string& value);

	std::size_t WriteString(const std::string& value, std::size_t size);

	virtual std::size_t Seek(std::size_t pos, bool relative = false);

	virtual std::size_t Tell() const;

protected:
	size_t size_{};
};

class OutputFileStream : public OutputStream
{
public:
	OutputFileStream(std::ofstream& stream);

	std::size_t Write(const void* data, std::size_t len) override;

	std::size_t Seek(std::size_t pos, bool relative = false) override;

	std::size_t Tell() const override;

private:
	std::ofstream& stream_;
};

class OutputMemoryStream : public OutputStream
{
public:
	OutputMemoryStream(std::vector<unsigned char>& buffer);

	std::size_t Write(const void* data, std::size_t len) override;

	std::size_t Seek(std::size_t pos, bool relative = false) override;

	std::size_t Tell() const override;

private:
	std::vector<unsigned char>& buffer_;

	size_t pos_{};	
};

}