#pragma once

#include <vector>
#include <filesystem>

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

struct DebugLine
{
	Vector3F startPos_;
	Vector3F endPos_;
};

class DebugMesh
{	
public:
	DebugMesh() = default;

	explicit DebugMesh(std::vector<Vector3F>&& vertices, std::vector<int32_t>&& indices);

	void Dump(const std::filesystem::path& path);

	void AddLine(const Vector3F& startPos, const Vector3F& endPos);

	void AddVertex(const Vector3F& pos);

	std::vector<Vector3F> GetTriangleList() const;	

private:
	std::vector<Vector3F> vertices_;

	std::vector<std::int32_t> indices_;

	std::vector<DebugLine> lines_;
};

}