#include <fstream>

#include "../utils/DebugMesh.h"

#include <spdlog/spdlog.h>

namespace WorldAssistant
{

DebugMesh::DebugMesh(std::vector<Vector3F>&& vertices, std::vector<int32_t>&& indices) :
	vertices_(std::move(vertices)),
	indices_(std::move(indices))
{
}

void DebugMesh::Dump(const std::filesystem::path& path)
{
	// Write to .obj
	std::ofstream stream(path);
	if (!stream.is_open()) {
		return;
	}

	stream << "# MTA:SA Detour\n";	

	for (const auto& vert : vertices_) {
		stream << "v " << vert.x_ << " " << vert.y_ << " " << vert.z_ << "\n";
	}		

	const size_t facesNum = indices_.size() / 3;
	for (size_t i = 0; i < facesNum; ++i) {
		stream << "f " << indices_[i * 3] + 1 << " " << indices_[i * 3 + 1] + 1 << " " << indices_[i * 3 + 2] + 1 << "\n";
	}

	for (const auto& line : lines_) {
		stream << "v " << line.startPos_.x_ << " " << line.startPos_.y_ << " " << line.startPos_.z_ << "\n";
		stream << "v " << line.endPos_.x_ << " " << line.endPos_.y_ << " " << line.endPos_.z_ << "\n";

		stream << "l -1 -2\n";
	}
}

void DebugMesh::AddLine(const Vector3F& startPos, const Vector3F& endPos)
{
	try {
		lines_.push_back({startPos, endPos});
	}
	catch ([[maybe_unused]] const std::bad_alloc& e) {
		spdlog::error("Out of memory");
	}
}

void DebugMesh::AddVertex(const Vector3F& pos)
{
	try {
		indices_.push_back(static_cast<int32_t>(vertices_.size()));
		vertices_.push_back(pos);
	}
	catch ([[maybe_unused]] const std::bad_alloc& e) {
		spdlog::error("Out of memory");
	}
}

std::vector<Vector3F> DebugMesh::GetTriangleList() const
{
	if (indices_.size() % 3 != 0) {
		return {};
	}

	std::vector<Vector3F> result;

	try {
		result.reserve(indices_.size() * sizeof(Vector3F));
	}
	catch ([[maybe_unused]] const std::bad_alloc& e) {
		spdlog::error("Out of memory");
		return {};
	}

	for (size_t i = 0; i < indices_.size(); i += 3) {
		const int32_t a = indices_[i];
		const int32_t b = indices_[i + 1];
		const int32_t c = indices_[i + 2];

		result.push_back(vertices_[a]);
		result.push_back(vertices_[b]);
		result.push_back(vertices_[c]);
	}

	return result;
}

}