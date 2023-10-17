#include "../scene/Scene.h"
#include "../scene/World.h"

#include <fstream>

#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace WorldAssistant
{

BoundingBox ApplyTransform(const BoundingBox& bounds, const glm::mat4& transform)
{
    Vector3F corners[8];
    corners[0] = bounds.min_;
    corners[1] = Vector3F(bounds.min_.x_, bounds.min_.y_, bounds.max_.z_);
    corners[2] = Vector3F(bounds.min_.x_, bounds.max_.y_, bounds.min_.z_);
    corners[3] = Vector3F(bounds.max_.x_, bounds.min_.y_, bounds.min_.z_);
    corners[4] = Vector3F(bounds.min_.x_, bounds.max_.y_, bounds.max_.z_);
    corners[5] = Vector3F(bounds.max_.x_, bounds.min_.y_, bounds.max_.z_);
    corners[6] = Vector3F(bounds.max_.x_, bounds.max_.y_, bounds.min_.z_);
    corners[7] = bounds.max_;

    BoundingBox result;
    for (size_t i = 0; i < 8; ++i) {
        const glm::vec4 transformed = transform * glm::vec4(corners[i].x_, corners[i].y_, corners[i].z_, 1);
        result.Merge(Vector3F(transformed.x, transformed.y, transformed.z));
    }

    return result;
}

/*
    SceneNode
*/
void SceneNode::SetPosition(const glm::vec3& pos)
{
	transform_[3][0] = pos.x;
	transform_[3][1] = pos.y;
	transform_[3][2] = pos.z;
    dirty_ = true;
}

void SceneNode::SetRotation(const glm::vec3& rot)
{
	glm::mat4 rotMat = glm::yawPitchRoll(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z));
	rotMat[3][0] = transform_[3][0];
	rotMat[3][1] = transform_[3][1];
	rotMat[3][2] = transform_[3][2];

	transform_ = rotMat;
    dirty_ = true;
}

void SceneNode::SetRotation(const glm::quat& rot)
{
	glm::mat4 rotMat = glm::toMat4(rot);
	rotMat[3][0] = transform_[3][0];
	rotMat[3][1] = transform_[3][1];
	rotMat[3][2] = transform_[3][2];

	transform_ = rotMat;
    dirty_ = true;
}

Scene::Scene(World* world) : 
    owner_(world),
    tree_(Rect(-5000, -5000, 5000, 5000))
{
}

bool Scene::Load(const std::filesystem::path& path)
{
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if (!result) {
        spdlog::error("Invalid scene file");
        return false;
    }

    const auto root = doc.child("map");
    if (!root) {
        spdlog::error("Invalid defs file");
        return false;
    }

    bounds_.Clear();
 
    for (pugi::xml_node tool = root.child("entry"); tool; tool = tool.next_sibling("entry"))
    {
        const uint32_t model = tool.attribute("model").as_uint();     

        glm::quat quat;
        quat.w = tool.attribute("rotW").as_float();
        quat.x = tool.attribute("rotX").as_float();
        quat.y = tool.attribute("rotY").as_float();
        quat.z = tool.attribute("rotZ").as_float();

        glm::mat4 transform = glm::toMat4(quat);
        transform[3][0] = tool.attribute("posX").as_float();
	    transform[3][1] = tool.attribute("posY").as_float();
	    transform[3][2] = tool.attribute("posZ").as_float();       

        AddNode(model, transform);       
    }

    return true;
}

bool Scene::Save(const std::filesystem::path& filename)
{
    const auto parentPath = filename.parent_path();

	// Create directory if not exists
	if (!std::filesystem::exists(parentPath)) {
		std::filesystem::create_directories(parentPath);
	}

	pugi::xml_document doc;
	pugi::xml_node mapNode = doc.append_child("map");

    for (SceneNode* node = nodes_.First(); node != nullptr; node = nodes_.Next(node)) {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(node->GetTransform(), scale, rotation, translation, skew, perspective);

        pugi::xml_node entryNode = mapNode.append_child("entry");
		entryNode.append_attribute("model") = node->GetModel();
		entryNode.append_attribute("posX") = translation.x;
		entryNode.append_attribute("posY") = translation.y;
		entryNode.append_attribute("posZ") = translation.z;
		entryNode.append_attribute("rotX") = rotation.x;
		entryNode.append_attribute("rotY") = rotation.y;
		entryNode.append_attribute("rotZ") = rotation.z;
		entryNode.append_attribute("rotW") = rotation.w;
    }

    std::ofstream stream(filename);
	doc.save(stream);

    return true;
}

SceneNode* Scene::AddNode(uint32_t model, const glm::mat4& transform)
{
    const auto* collision = owner_->GetModelCollision(model);
    if (!collision) {
        spdlog::error("Could not find a collsion for model {}", model);
        return {};
    }

    const auto bounds = ApplyTransform(collision->GetBounds().aabb_, transform);

    SceneNode* node = new SceneNode;
    node->model_ = model;
    node->transform_ = transform;
    node->box_.Define(Vector2F(bounds.min_.x_, bounds.min_.z_), 
        Vector2F(bounds.max_.x_, bounds.max_.z_));
    node->dirty_ = false;
 
    bounds_.Merge(bounds);        
    nodes_.InsertFront(node);
    tree_.Add(node);

    return node;
}

void Scene::RemoveNode(SceneNode* node)
{
}

void Scene::Query(float* bounds, std::vector<const SceneNode*>& result)
{
    result.clear();

    auto results = tree_.Query(Rect(bounds[0], bounds[2], bounds[3], bounds[5]));
    for (const auto& entry : results) {
        result.push_back(static_cast<const SceneNode*>(entry));
    }
}

bool Scene::Empty() const
{
    return false;
}

}