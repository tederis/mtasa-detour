#include <iostream>
#include <array>
#include <regex>

#include "../game/IMGArchive.h"

namespace WorldAssistant
{

IMGArchive::IMGArchive()
{
}

IMGArchive::~IMGArchive()
{
	if (stream_.is_open()) {
		stream_.close();
	}
}

bool IMGArchive::Open(const std::filesystem::path& filename)
{
	// It's already opened
	if (IsOpen()) {
		return false;
	}

	std::ifstream stream(filename, std::ios_base::in | std::ios_base::binary);
	if (!stream.is_open()) {
		return false;
	}

	char version[4];
	stream.read(version, 4);

	if (std::string_view(version, 4) != "VER2") {		
		return false;
	}

	unsigned entriesNum;
	stream.read((char*)&entriesNum, sizeof(entriesNum));

	entries_.reserve(1024);

	for (unsigned i = 0; i < entriesNum; ++i) {
		IMGEntry entry;
		stream.read((char*)&entry, sizeof(entry));
		entries_.push_back(std::move(entry));
	}

	entries_.shrink_to_fit();
	stream_ = std::move(stream);

	return true;
}

void IMGArchive::Close()
{
	if (stream_.is_open()) {
		stream_.close();
	}
}

bool IMGArchive::IsOpen() const
{
	return stream_.is_open();
}

void IMGArchive::ForEach(const std::string& mask, std::function<void(const IMGEntry&)> fn) noexcept
{
	const std::regex rx(mask);

	for (const auto& entry : entries_) {
		if (std::regex_match(entry.name_, rx)) {
			fn(entry);
		}
	}
}

bool IMGArchive::Read(std::vector<std::uint8_t>& buffer, std::size_t offset, std::size_t size)
{
	if (!IsOpen()) {
		return false;
	}

	buffer.resize(size * 2048);

	stream_.seekg(offset * 2048);
	stream_.read(reinterpret_cast<char*>(buffer.data()), size * 2048);

	return true;
}

}