#pragma once

#include <filesystem>
#include <array>
#include <memory>
#include <fstream>
#include <functional>
#include <cstring>

namespace WorldAssistant
{

struct IMGEntry
{
	IMGEntry() = default;

	IMGEntry(const IMGEntry& entry) :
		offset_(entry.offset_),
		streamingSize_(entry.streamingSize_),
		archiveSize_(entry.archiveSize_)
	{
		std::memcpy(name_, entry.name_, 24);
	}

	IMGEntry& operator= (const IMGEntry& entry)
	{
		offset_ = entry.offset_;
		streamingSize_ = entry.streamingSize_;
		archiveSize_ = entry.archiveSize_;
		std::memcpy(name_, entry.name_, 24);
		return *this;
	}


	unsigned int offset_{};

	unsigned short streamingSize_{};

	unsigned short archiveSize_{};

	char name_[24]{};
};

class IMGArchive;

class IMGArchiveView
{
public:
	explicit IMGArchiveView() :
		offset_(0),
		size_(0)
	{
	}

	explicit IMGArchiveView(const std::shared_ptr<IMGArchive>& archive, unsigned offset, unsigned size) :
		archive_(archive),
		offset_(offset),
		size_(size)
	{
	}

private:
	std::shared_ptr<IMGArchive> archive_;

	unsigned offset_;

	unsigned size_;
};

class IMGArchive
{
public:
	IMGArchive();

	~IMGArchive();

	bool Open(const std::filesystem::path& filename);

	void Close();

	bool IsOpen() const;

	void ForEach(const std::string& mask, std::function<void(const IMGEntry&)> fn) noexcept;

	bool Read(std::vector<std::uint8_t>& buffer, std::size_t offset, std::size_t size);

	const std::vector<IMGEntry>& GetEntries() const { return entries_; }

private:
	std::ifstream stream_;

	std::vector<IMGEntry> entries_;
};

}