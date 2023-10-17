#pragma once

#include <vector>
#include <optional>
#include <unordered_set>
#include <map>
#include <filesystem>

#include <glm/glm.hpp>

#include "../utils/MathUtils.h"
#include "../utils/UtilsStream.h"

namespace WorldAssistant
{

enum class CollisionVer
{
	COLL = 0,
	COL2,
	COL3
};

struct ColBoundingBox
{
	BoundingBox aabb_;
	Vector3F center_;
	float radius_;
};
static_assert(sizeof(ColBoundingBox) == 40, "Invalid size of ColBoundingBox");

using ColVertex = Int16Vector3;
static_assert(sizeof(ColVertex) == 6, "Invalid size of ColVertex");

struct ColFace
{
	ColFace() = default;

	ColFace(uint16_t a, uint16_t b, uint16_t c, uint8_t mat = 0u, uint8_t light = 0u) :
		a_(a), b_(b), c_(c), mat_(mat), light_(light)
	{
	}

	uint16_t a_{};
	uint16_t b_{};
	uint16_t c_{};
	uint8_t mat_{};
	uint8_t light_{};
};
static_assert(sizeof(ColFace) == 8, "Invalid size of ColFace");

struct ColBox
{
	float minx_{};
	float miny_{};
	float minz_{};
	float maxx_{};
	float maxy_{};
	float maxz_{};
	uint8_t mat_{};
	uint8_t flag_{};
	uint8_t unknown_{};
	uint8_t light_{};
};
static_assert(sizeof(ColBox) == 28, "Invalid size of ColBox");

struct CollisionModifier
{
	const std::unordered_set<uint8_t>& ignoredMaterials_;
};

class Collision
{
public:
	std::optional<unsigned> Load(InputStream& stream);

	bool Save(OutputStream& output);

	void Unpack(std::vector<Vector3F>& vertices, std::vector<int>& indices, const glm::mat4& transform, size_t startIndex = {}, bool clear = false) const;

	std::size_t ApplyModifier(const CollisionModifier& modifier);

	bool Empty() const;

	const std::vector<ColFace>& GetFaces() const { return faces_; }

	const std::vector<ColVertex>& GetVertices() const { return vertices_; }

	const ColBoundingBox& GetBounds() const { return bounds_; }

	const std::string& GetName() const { return name_; }

private:
	std::vector<ColFace> faces_;

	std::vector<ColVertex> vertices_;

	ColBoundingBox bounds_;

	std::string name_;

	CollisionVer version_{};
};

class CollisionFile
{
public:
	CollisionFile();

	bool Load(InputStream& input);

	bool Load(const std::filesystem::path& path);

	bool Save(OutputStream& output);

	bool Save(const std::filesystem::path& path);

	void Insert(CollisionFile* collisions);

	std::size_t ApplyModifier(const CollisionModifier& modifier);

	const Collision* GetCollision(const std::string& name) const;

	const std::map<std::string, std::shared_ptr<Collision>>& GetEntries() const { return collisions_; }

	std::size_t GetCollisionsNum() const { return collisions_.size(); }

private:
	std::map<std::string, std::shared_ptr<Collision>> collisions_;
};

}