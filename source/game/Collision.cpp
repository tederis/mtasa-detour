#include "../game/Collision.h"

#include <spdlog/spdlog.h>

#define PACK_COORDINATE(x) (static_cast<int16_t>(x * 128.0f))
#define UNPACK_COORDINATE(x) (static_cast<float>(static_cast<float>(x) / 128.0f))

namespace WorldAssistant
{

namespace
{
    std::size_t GetIndicesNum(const std::vector<ColFace>& faces)
    {
        if (faces.size() < 1) {
            return 0u;
        }

        uint16_t maxIndex{};

        for (const auto& face : faces) {
            maxIndex = std::max(face.a_, maxIndex);
            maxIndex = std::max(face.b_, maxIndex);
            maxIndex = std::max(face.c_, maxIndex);
        }

        return static_cast<std::size_t>(maxIndex + 1);
    }

    void PushCollisionBox(const ColBox& box, std::vector<ColVertex>& vertices, std::vector<ColFace>& faces)
    {
        if (vertices.size() + 7 >= std::numeric_limits<uint16_t>().max()) {
            return;
        }

        const uint16_t lastIndex = static_cast<uint16_t>(vertices.size());

        ColVertex v0{ PACK_COORDINATE(box.minx_), PACK_COORDINATE(box.miny_), PACK_COORDINATE(box.minz_) };
        ColVertex v1{ PACK_COORDINATE(box.minx_), PACK_COORDINATE(box.maxy_), PACK_COORDINATE(box.minz_) };
        ColVertex v2{ PACK_COORDINATE(box.maxx_), PACK_COORDINATE(box.maxy_), PACK_COORDINATE(box.minz_) };
        ColVertex v3{ PACK_COORDINATE(box.maxx_), PACK_COORDINATE(box.miny_), PACK_COORDINATE(box.minz_) };

        ColVertex v4{ PACK_COORDINATE(box.minx_), PACK_COORDINATE(box.miny_), PACK_COORDINATE(box.maxz_) };
        ColVertex v5{ PACK_COORDINATE(box.minx_), PACK_COORDINATE(box.maxy_), PACK_COORDINATE(box.maxz_) };
        ColVertex v6{ PACK_COORDINATE(box.maxx_), PACK_COORDINATE(box.maxy_), PACK_COORDINATE(box.maxz_) };
        ColVertex v7{ PACK_COORDINATE(box.maxx_), PACK_COORDINATE(box.miny_), PACK_COORDINATE(box.maxz_) };    

        vertices.push_back(std::move(v0));
        vertices.push_back(std::move(v1));
        vertices.push_back(std::move(v2));
        vertices.push_back(std::move(v3));
        vertices.push_back(std::move(v4));
        vertices.push_back(std::move(v5));
        vertices.push_back(std::move(v6));
        vertices.push_back(std::move(v7));    

        faces.emplace_back(lastIndex + 0, lastIndex + 4, lastIndex + 7, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 0, lastIndex + 7, lastIndex + 3, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 3, lastIndex + 7, lastIndex + 6, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 3, lastIndex + 6, lastIndex + 2, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 2, lastIndex + 6, lastIndex + 5, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 2, lastIndex + 5, lastIndex + 1, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 1, lastIndex + 5, lastIndex + 4, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 1, lastIndex + 4, lastIndex + 0, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 4, lastIndex + 5, lastIndex + 6, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 4, lastIndex + 6, lastIndex + 7, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 0, lastIndex + 3, lastIndex + 2, box.mat_, box.light_);
        faces.emplace_back(lastIndex + 0, lastIndex + 2, lastIndex + 1, box.mat_, box.light_);
    }
}

/*
    Collision
*/
std::optional<unsigned> Collision::Load(InputStream& stream)
{
    const std::string version = stream.ReadFileID();
    const auto fileBegin = stream.Tell();

    if (version == "COLL") {
        // COLL is not supported
        return std::nullopt;
    }
    else if (version == "COL2") {
        version_ = CollisionVer::COL2;
    }
    else if (version == "COL3") {
        version_ = CollisionVer::COL3;
    }
    else {
        return std::nullopt;
    }

    const unsigned int fileSize = stream.ReadUInt();

    char modelName[22];
    stream.Read(modelName, 22);
    name_.assign(modelName);

    const unsigned short modelIndex = stream.ReadUShort();

    stream.Read(&bounds_, sizeof(ColBoundingBox));

    if (version_ >= CollisionVer::COL2) {
        unsigned short spheresNum = stream.ReadUShort();
        unsigned short boxesNum = stream.ReadUShort();
        unsigned short facesNum = stream.ReadUShort();
        unsigned char wheelsNum = stream.ReadUByte();
        unsigned int flags = stream.ReadUInt();
        stream.Seek(1, true);
        unsigned int offsetSpheres = stream.ReadUInt();
        unsigned int offsetBoxes = stream.ReadUInt();
        unsigned int offsetLines = stream.ReadUInt();
        unsigned int offsetVertices = stream.ReadUInt();
        unsigned int offsetFaces = stream.ReadUInt();
        unsigned int offsetPlanes = stream.ReadUInt();
        unsigned int shadowNum{};
        unsigned int offsetShadowVerties{};
        unsigned int offsetShadowFaces{};

        if (version_ >= CollisionVer::COL3) {
            shadowNum = stream.ReadUInt();
            offsetShadowVerties = stream.ReadUInt();
            offsetShadowFaces = stream.ReadUInt();   
        }

        if (/*flags & 2*/ true) {
            if (facesNum > 0) {
                stream.Seek(static_cast<unsigned>(fileBegin) + offsetFaces);     

                faces_.resize(facesNum);
                stream.Read(faces_.data(), sizeof(ColFace) * facesNum);
            }

            const auto verticesNum = GetIndicesNum(faces_);
            if (verticesNum > 0) {
                stream.Seek(static_cast<unsigned>(fileBegin) + offsetVertices);

                vertices_.resize(verticesNum);
                stream.Read(vertices_.data(), sizeof(ColVertex) * verticesNum);
            }

            if (boxesNum > 0) {
                stream.Seek(static_cast<unsigned>(fileBegin) + offsetBoxes);

                ColBox box;
                for (size_t i = 0; i < boxesNum; ++i) {
                    stream.Read(&box, sizeof(ColBox));
                    PushCollisionBox(box, vertices_, faces_);
                }                
            }
        }
    }

    for (auto& vert : vertices_) {
        std::swap(vert.y_, vert.z_);
    }

    BoundingBox aabb;
    aabb.Merge(Vector3F(bounds_.aabb_.min_.x_, bounds_.aabb_.min_.z_, bounds_.aabb_.min_.y_));
    aabb.Merge(Vector3F(bounds_.aabb_.max_.x_, bounds_.aabb_.max_.z_, bounds_.aabb_.max_.y_));
    bounds_.aabb_ = aabb;

    return fileSize + 8;
}

bool Collision::Save(OutputStream& output)
{
    if (Empty()) {
        return false;
    }

    std::size_t fileSize = 22 + 2 + 40 + 36;
    fileSize += faces_.size() * sizeof(ColFace);
    fileSize += vertices_.size() * sizeof(ColVertex);

    ColBoundingBox bounds;
    bounds.aabb_.Merge(Vector3F(bounds_.aabb_.min_.x_, bounds_.aabb_.min_.z_, bounds_.aabb_.min_.y_));
    bounds.aabb_.Merge(Vector3F(bounds_.aabb_.max_.x_, bounds_.aabb_.max_.z_, bounds_.aabb_.max_.y_));
    bounds.center_ = bounds_.center_;
    bounds.radius_ = bounds_.radius_;

    output.WriteFileID("COL2");    
    output.WriteUInt(fileSize);
    output.WriteString(name_, 22);
    output.WriteUShort(0);
    output.Write(&bounds, sizeof(ColBoundingBox));

    output.WriteUShort(0); // Spheres
    output.WriteUShort(0); // Boxes
    output.WriteUShort(static_cast<uint16_t>(faces_.size())); // Faces
    output.WriteUByte(0); // Wheel num
    output.WriteUInt(0); // Flags
    output.WriteUByte(0); // Unknown
    output.WriteUInt(0); // Offset of spheres
    output.WriteUInt(0); // Offset of boxes
    output.WriteUInt(0); // Offset of lines
    output.WriteUInt(22 + 2 + 40 + 36 + 4); // Offset of vertices
    output.WriteUInt(22 + 2 + 40 + 36 + 4 + vertices_.size() * sizeof(ColVertex)); // Offset of faces
    output.WriteUInt(0); // Offset of planes

    for (const auto& vertex : vertices_) {
        output.WriteShort(vertex.x_);
        output.WriteShort(vertex.z_);
        output.WriteShort(vertex.y_);
    }

    for (const auto& face : faces_) {
        output.WriteUShort(face.a_);
        output.WriteUShort(face.b_);
        output.WriteUShort(face.c_);
        output.WriteUByte(face.mat_);
        output.WriteUByte(face.light_);
    }

    return true;
}

void Collision::Unpack(std::vector<Vector3F>& vertices, std::vector<int>& indices, const glm::mat4& transform, size_t startIndex, bool clear) const
{
    if (clear) {
        if (vertices.capacity() < vertices_.size()) {
            vertices.reserve(vertices_.size());
        }

        if (indices.capacity() < faces_.size() * 3) {
            indices.reserve(faces_.size() * 3);
        }

        vertices.clear();
        indices.clear();
    }

    for (const auto& vertex : vertices_) {
        glm::vec4 pos(UNPACK_COORDINATE(vertex.x_), UNPACK_COORDINATE(vertex.y_), UNPACK_COORDINATE(vertex.z_), 1.0f);
        pos = transform * pos;

        vertices.push_back(Vector3F(pos.x, pos.y, pos.z));
    }

    for (const auto& face : faces_) {
        indices.push_back(startIndex + static_cast<int>(face.a_));
        indices.push_back(startIndex + static_cast<int>(face.b_));
        indices.push_back(startIndex + static_cast<int>(face.c_));
    }
}

std::size_t Collision::ApplyModifier(const CollisionModifier& modifier)
{
    return std::erase_if(faces_, [&modifier](const ColFace& face) {
        return modifier.ignoredMaterials_.contains(face.mat_);
    });
}

bool Collision::Empty() const
{
    return faces_.empty() || vertices_.empty();
}

/*
    CollisionFile
*/
CollisionFile::CollisionFile()
{
}

bool CollisionFile::Load(InputStream& input)
{
    // Reset previous entries
    collisions_.clear();
  
    
    while (!input.Eof()) {
        const auto filePos = input.Tell();

        std::shared_ptr<Collision> collision = std::make_shared<Collision>();

        auto fileSize = collision->Load(input);
        if (!fileSize.has_value())
            break;

        collisions_[collision->GetName()] = collision;

        // Restore position
        input.Seek(filePos);
        input.Seek(fileSize.value(), true);      
    }

    return true;
}

bool CollisionFile::Load(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::in);
	if (stream.is_open())
	{
		InputFileStream output(stream);
		return Load(output);
	}

	return false;
}

bool CollisionFile::Save(OutputStream& output)
{
    for (const auto& [name, entry] : collisions_) {
        entry->Save(output);
    }

    return true;
}

bool CollisionFile::Save(const std::filesystem::path& path)
{
    std::ofstream stream(path, std::ios::binary | std::ios::out);
	if (stream.is_open())
	{
		OutputFileStream output(stream);
		return Save(output);
	}

	return false;
}

void CollisionFile::Insert(CollisionFile* collisions)
{
    for (const auto& [name, collision] : collisions->GetEntries()) {
        collisions_[name] = collision;
    }
}

std::size_t CollisionFile::ApplyModifier(const CollisionModifier& modifier)
{
    std::size_t count{};

    for (auto it = collisions_.begin(); it != collisions_.end();) {
        Collision* collision = it->second.get();
        count += collision->ApplyModifier(modifier);

        if (collision->Empty()) {
            // Erase empty collisions
            it = collisions_.erase(it);
        }
        else {
            ++it;
        }
    }

    return count;
}

const Collision* CollisionFile::GetCollision(const std::string& name) const
{
    if (auto found = collisions_.find(name); found != collisions_.end()) {
		return found->second.get();
	}

    return {};
}

}