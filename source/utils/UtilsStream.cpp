#include <assert.h>
#include <cstring>

#include "../utils/UtilsStream.h"

namespace WorldAssistant
{

/*
	InputStream
*/
InputStream::InputStream()
{	
}

std::size_t InputStream::Read(void* data, std::size_t len)
{
	return 0u;
}

std::string InputStream::ReadFileID()
{
	char buffer[4];
	Read(buffer, 4u);
	return std::string(buffer, 4u);
}

std::uint8_t InputStream::ReadUByte()
{
	std::uint8_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::uint8_t));
	return value;
}

std::int32_t InputStream::ReadInt()
{
	std::int32_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::int32_t));
	return value;
}

std::uint32_t InputStream::ReadUInt()
{
	std::uint32_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::uint32_t));
	return value;
}

std::uint64_t InputStream::ReadUInt64()
{
	std::uint64_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::uint64_t));
	return value;
}

float InputStream::ReadFloat()
{
	float value;
	Read(reinterpret_cast<char*>(&value), sizeof(float));
	return value;
}

Vector3F InputStream::ReadVector3()
{
	Vector3F value;
	Read(reinterpret_cast<char*>(&value.x_), sizeof(Vector3F));
	return value;
}

BoundingBox InputStream::ReadBoundingBox()
{
	const Vector3F min = ReadVector3();
	const Vector3F max = ReadVector3();
	return BoundingBox(min, max);
}

std::int16_t InputStream::ReadShort()
{
	std::int16_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::int16_t));
	return value;
}

std::uint16_t InputStream::ReadUShort()
{
	std::uint16_t value;
	Read(reinterpret_cast<char*>(&value), sizeof(std::uint16_t));
	return value;
}

std::string InputStream::ReadString()
{
	std::string value;
	while (char ch = static_cast<char>(ReadUByte()))
		value += ch;

	return value;
}

std::size_t InputStream::Seek(std::size_t pos, bool relative)
{
	return 0u;
}

std::size_t InputStream::Tell() const
{
	return 0u;
}

bool InputStream::Eof() const
{
	return true;
}

/*
	InputFileStream
*/
InputFileStream::InputFileStream(std::ifstream& stream) :
	stream_(stream)
{
	const auto pos = stream.tellg();

	stream.seekg(0, std::ios::end);
	size_ = static_cast<std::size_t>(stream.tellg());
	stream.seekg(pos);
}

std::size_t InputFileStream::Read(void* data, std::size_t len)
{
	stream_.read((char*)data, len);
	return len * sizeof(uint8_t);
}

std::size_t InputFileStream::Seek(std::size_t pos, bool relative)
{
	stream_.seekg(pos, relative ? std::ios::cur : std::ios::beg);
	return static_cast<std::size_t>(stream_.tellg());
}

std::size_t InputFileStream::Tell() const
{
	return static_cast<std::size_t>(stream_.tellg());
}

bool InputFileStream::Eof() const
{
	return stream_.peek() == EOF;
}

/*
	InputMemoryStream
*/
InputMemoryStream::InputMemoryStream(const std::vector<unsigned char>& buffer) :
	InputMemoryStream(buffer.data(), buffer.size())
{	
}

InputMemoryStream::InputMemoryStream(const std::uint8_t* buffer, std::size_t size)
{
	buffer_ = buffer;
	size_ = size;
}

std::size_t InputMemoryStream::Read(void* data, std::size_t len)
{
	if (len + pos_ > size_)
        len = size_ - pos_;

    if (!len)
        return 0u;

    const unsigned char* srcPtr = buffer_ + pos_;
    auto* destPtr = (unsigned char*)data;
    pos_ += len;

    memcpy(destPtr, srcPtr, len);

    return len;
}

std::size_t InputMemoryStream::Seek(std::size_t pos, bool relative)
{
	if (relative) {
		pos = pos_ + pos; 
	}

	if (pos > size_) {
        pos = size_;
	}

    pos_ = pos;
    return pos_;
}

std::size_t InputMemoryStream::Tell() const
{
	return pos_;
}

bool InputMemoryStream::Eof() const
{
	return pos_ >= size_;
}

/*
	OutputStream
*/
OutputStream::OutputStream()
{
}

std::size_t OutputStream::Write(const void* data, std::size_t len)
{
	return 0;
}

std::size_t OutputStream::WriteFileID(const char* value)
{
	assert(strlen(value) >= 4);
	return Write(value, 4);
}

std::size_t OutputStream::WriteBool(bool value)
{
	return WriteUByte(value ? 1 : 0);
}

std::size_t OutputStream::WriteUByte(std::uint8_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::uint8_t));
}

std::size_t OutputStream::WriteUInt(std::uint32_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::uint32_t));
}

std::size_t OutputStream::WriteUInt64(std::uint64_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::uint64_t));
}

std::size_t OutputStream::WriteInt(std::int32_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::int32_t));
}

std::size_t OutputStream::WriteUShort(std::uint16_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::uint16_t));
}

std::size_t OutputStream::WriteShort(std::int16_t value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(std::int16_t));
}

std::size_t OutputStream::WriteFloat(float value)
{
	return Write(reinterpret_cast<const char*>(&value), sizeof(float));
}

std::size_t OutputStream::WriteVector3(const Vector3F& value)
{
	return Write(reinterpret_cast<const char*>(&value.x_), sizeof(Vector3F));
}

std::size_t OutputStream::WriteBoundingBox(const BoundingBox& bounds)
{
	WriteVector3(bounds.min_);
	WriteVector3(bounds.max_);
	return sizeof(BoundingBox);
}

std::size_t OutputStream::WriteString(const std::string& value)
{
	return Write(value.c_str(), sizeof(uint8_t) * (value.length() + 1u));
}

std::size_t OutputStream::WriteString(const std::string& value, std::size_t size)
{
	const std::size_t lengthPlusOne = value.length() + 1u; // String len + null terminator
	Write(value.c_str(), sizeof(uint8_t) * std::min(lengthPlusOne, size));
	
	if (size > lengthPlusOne) {
		for (std::size_t i = 0; i < size - lengthPlusOne; ++i) {
			WriteUByte(0);
		}
	}

	return sizeof(uint8_t) * size;
}

std::size_t OutputStream::Seek(std::size_t pos, bool relative)
{
	return 0u;
}

std::size_t OutputStream::Tell() const
{
	return 0u;
}

/*
	OutputFileStream
*/
OutputFileStream::OutputFileStream(std::ofstream& stream) :
	stream_(stream)
{
}

std::size_t OutputFileStream::Write(const void* data, std::size_t len)
{
	stream_.write((const char*)data, len * sizeof(uint8_t));
	return len * sizeof(uint8_t);
}

std::size_t OutputFileStream::Seek(std::size_t pos, bool relative)
{
	stream_.seekp(pos, relative ? std::ios::cur : std::ios::beg);
	return static_cast<std::size_t>(stream_.tellp());
}

std::size_t OutputFileStream::Tell() const
{
	return static_cast<std::size_t>(stream_.tellp());
}

/*
	OutputMemoryStream
*/
OutputMemoryStream::OutputMemoryStream(std::vector<unsigned char>& buffer) :
	buffer_(buffer)
{
}

std::size_t OutputMemoryStream::Write(const void* data, std::size_t len)
{
	if (!len)
        return 0;

    if (len + pos_ > size_)
    {
        size_ = len + pos_;

		try	{
			buffer_.resize(size_);
		}
		catch ([[maybe_unused]] const std::bad_alloc& e) {
			return 0u;
		}
    }

    auto* srcPtr = (unsigned char*)data;
    unsigned char* destPtr = buffer_.data() + pos_;
    pos_ += len;

    memcpy(destPtr, srcPtr, len);

    return len;
}

std::size_t OutputMemoryStream::Seek(std::size_t pos, bool relative)
{
	if (pos > size_)
        pos = size_;

    pos_ = pos;
    return pos_;
}

std::size_t OutputMemoryStream::Tell() const
{
	return pos_;
}

}